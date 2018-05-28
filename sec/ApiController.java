package com._44bz.wxgzh.api.controller;

import com._44bz.tool.SoFast;
import com._44bz.wxgzh.api.bean.User;
import com._44bz.wxgzh.api.config.SysConfigBean;
import com._44bz.wxgzh.api.config.WechatMpProperties;
import com._44bz.wxgzh.api.service.IOrderService;
import com._44bz.wxgzh.api.service.IUserService;
import com._44bz.wxgzh.api.utils.WXPayUtil;
import me.chanjar.weixin.common.bean.WxJsapiSignature;
import me.chanjar.weixin.common.exception.WxErrorException;
import me.chanjar.weixin.mp.api.WxMpService;
import me.chanjar.weixin.mp.bean.result.WxMpOAuth2AccessToken;
import org.apache.catalina.servlet4preview.http.HttpServletRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.ModelAndView;

import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by Dylan on 2018/1/30.
 */
@RestController
public class ApiController {
    private static final Logger LOG = LoggerFactory.getLogger(ApiController.class);

    @Autowired
    private SysConfigBean sysConfigBean;

    @Autowired
    private WxMpService wxService;
    @Autowired
    private IUserService userService;
    @Autowired
    private IOrderService orderService;
    @Autowired
    private WechatMpProperties wechatMpProperties;

    @RequestMapping("/webpage")
    public ModelAndView mall() {
        return new ModelAndView("home");
    }

    @RequestMapping("/WxApi")
    public String WxApi(String page_url) {
//        config.appId = json.data.appId;
//        config.timestamp = json.data.timestamp;
//        config.nonceStr = json.data.nonceStr;
//        config.signature = json.data.signature;

        try {
            WxJsapiSignature jsapiSignature = wxService.createJsapiSignature(page_url);
            return Util.responseBody(0, "", jsapiSignature);
        } catch (WxErrorException e) {
            LOG.error("createJsapiSignature", e);
            return Util.responseBody(1, "", null);
        }
    }

    @RequestMapping("/wx")
    public String home(String signature, String timestamp, String nonce, String echostr) {
        String token = sysConfigBean.getWxscGzhToken();
        LOG.debug("signature {},token {},timestamp {},nonce {},echostr {}", signature, token, timestamp, nonce, echostr);
        if (SoFast.isNotBlank(signature, token, timestamp, nonce, echostr)) {
            String[] array = new String[]{token, timestamp, nonce};
            StringBuffer sb = new StringBuffer();
            // 字符串排序
            Arrays.sort(array);
            for (int i = 0; i < array.length; i++) {
                sb.append(array[i]);
            }
            String str = sb.toString();

            String sha1 = SHA1(str);
            LOG.debug("sha1 {}", sha1);
            if (signature.equals(sha1)) {
                return echostr;
            }
        }
        return "{err:1,msg:\"非法访问！\"}";
    }

    @RequestMapping("/wxpayNotify")
    public String wxpayNotify(@RequestBody String body, HttpServletRequest httpServletRequest, HttpServletResponse httpServletResponse) {
        HashMap<String, String> result = new HashMap<>();

        //==================================================================//
        //  支付结果通知                                                      //
        //  https://pay.weixin.qq.com/wiki/doc/api/jsapi.php?chapter=9_7    //
        //==================================================================//
        try {

            Map<String, String> bodyData = WXPayUtil.xmlToMap(body);
            if (WXPayUtil.SUCCESS.equals(bodyData.get("return_code")) && WXPayUtil.SUCCESS.equals(bodyData.get("result_code"))) {
                String appid = bodyData.get("appid");
                String mch_id = bodyData.get("mch_id");
                String openid = bodyData.get("openid");
                String transaction_id = bodyData.get("transaction_id");
                String out_trade_no = bodyData.get("out_trade_no");
                String time_end = bodyData.get("time_end");
                if (wechatMpProperties.getPayAppId().equals(appid) && wechatMpProperties.getMchId().equals(mch_id)) {

                    // TODO openid
                    User payUser = userService.findByOpenId("oTOyT06CzszaXrVkwOzhEvze5jVA");
//                    User payUser = userService.findByOpenId(openid);

                    // TODO 是否保存 time_end
                    orderService.payDone(out_trade_no, transaction_id, payUser);
                    result.put("return_code", WXPayUtil.SUCCESS);
                    result.put("return_msg", "OK");
                    return WXPayUtil.mapToXml(result);
                }
            }

            // fail
            result.put("return_code", WXPayUtil.FAIL);
            result.put("return_msg", "ERROR");
            return WXPayUtil.mapToXml(result);
        } catch (Exception e) {
            result.put("return_msg", "ERROR");
        }
        return "FAIL";
    }

    @RequestMapping("/authorization_callback")
    public String authorizationCallback(String code, HttpServletRequest httpServletRequest, HttpServletResponse httpServletResponse) {
        try {
            WxMpOAuth2AccessToken wxMpOAuth2AccessToken = wxService.oauth2getAccessToken(code);
            String openId = wxMpOAuth2AccessToken.getOpenId();
//            WxMpUser wxMpUser = wxService.getUserService().userInfo(openId);
            userService.addOpenIdIfAbsent(openId);
            httpServletRequest.getSession().setAttribute("currentUser", openId);
            httpServletResponse.sendRedirect("/");
            return null;
        } catch (WxErrorException | IOException e) {
            e.printStackTrace();
            httpServletRequest.getSession().removeAttribute("currentUser");
            LOG.error("authorization_callback 用户唯一标识获取失败");
            return "非法访问！";
        }

    }


    public static String SHA1(String decript) {
        try {
            MessageDigest digest = java.security.MessageDigest
                    .getInstance("SHA-1");
            digest.update(decript.getBytes());
            byte messageDigest[] = digest.digest();
            // Create Hex String
            StringBuffer hexString = new StringBuffer();
            // 字节数组转换为 十六进制 数
            for (int i = 0; i < messageDigest.length; i++) {
                String shaHex = Integer.toHexString(messageDigest[i] & 0xFF);
                if (shaHex.length() < 2) {
                    hexString.append(0);
                }
                hexString.append(shaHex);
            }
            return hexString.toString();

        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        return "";
    }
}
