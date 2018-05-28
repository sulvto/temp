package com._44bz.wxgzh.security.sec;

import com._44bz.exception.ThrbException;
import com._44bz.wxgzh.security.bean.Role;
import com._44bz.wxgzh.security.bean.User;
import com._44bz.wxgzh.security.service.IUserService;
import org.apache.commons.lang3.StringUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.core.GrantedAuthority;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.security.core.userdetails.UserDetailsService;
import org.springframework.security.core.userdetails.UsernameNotFoundException;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * @author David
 * Created by David on 2018/3/2.
 */
@Service
public class MongoDBUserService implements UserDetailsService {
    @Autowired
    private IUserService userService;

    @Override
    public UserDetails loadUserByUsername(String username) throws UsernameNotFoundException {
        User user = userService.findByUsername(username);
        if (user != null) {
            Set<Role> roles = user.getRoles();
            if (roles != null) {
                List<GrantedAuthority> authorities = roles.stream().flatMap(role -> role.getAuthorities().stream()).filter(authority -> StringUtils.isNotBlank(authority.getName())).map(authority -> new SimpleGrantedAuthority(authority.getName())).collect(Collectors.toList());
                authorities = Optional.ofNullable(authorities).orElse(new ArrayList<>());
                return new org.springframework.security.core.userdetails.User(user.getUsername(), user.getPassword(), authorities);
            }
            throw new ThrbException("login: " + username + " no role!");
        } else {
            throw new UsernameNotFoundException("login: " + username + " do not exist!");
        }
    }
}
