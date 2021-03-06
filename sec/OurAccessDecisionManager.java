package com._44bz.wxgzh.security.sec;

import org.springframework.security.access.AccessDecisionManager;
import org.springframework.security.access.AccessDeniedException;
import org.springframework.security.access.ConfigAttribute;
import org.springframework.security.authentication.InsufficientAuthenticationException;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.GrantedAuthority;
import org.springframework.stereotype.Service;

import java.util.Collection;
import java.util.Iterator;

/**
 * @author David
 * Created by David on 2018/3/2.
 */
@Service
public class OurAccessDecisionManager implements AccessDecisionManager {
    /**
     * decide 方法是判定是否拥有权限的决策方法
     *
     * @param authentication   是MongoDBUserService中循环添加到 GrantedAuthority 对象中的权限信息集合.
     * @param object           包含客户端发起的请求的requset信息，可转换为 HttpServletRequest request = ((FilterInvocation) object).getHttpRequest();
     * @param configAttributes 为OurInvocationSecurityMetadataSourceService的getAttributes(Object object)这个方法返回的结果，此方法是为了判定用户请求的url 是否在权限表中，如果在权限表中，则返回给 decide 方法，用来判定用户是否有此权限。如果不在权限表中则放行。
     * @throws AccessDeniedException
     * @throws InsufficientAuthenticationException
     */
    @Override
    public void decide(Authentication authentication, Object object, Collection<ConfigAttribute> configAttributes) throws AccessDeniedException, InsufficientAuthenticationException {
        if (null == configAttributes || configAttributes.size() <= 0) {
            return;
        }
        ConfigAttribute c;
        String needAuthority;
        for (Iterator<ConfigAttribute> configAttributeIterator = configAttributes.iterator(); configAttributeIterator.hasNext(); ) {
            c = configAttributeIterator.next();
            needAuthority = c.getAttribute();
            for (GrantedAuthority ga : authentication.getAuthorities()) {
                if (needAuthority.trim().equals(ga.getAuthority())) {
                    return;
                }
            }
        }
        throw new AccessDeniedException("no right");
    }

    @Override
    public boolean supports(ConfigAttribute attribute) {
        return true;
    }

    @Override
    public boolean supports(Class<?> clazz) {
        return true;
    }
}
