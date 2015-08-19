# Setup SSL Directory

Assume airweb's uid is 1012. Make sure that airweb can access your certs.

    $ mkdir ssl
    $ chmod 700 ssl
    $ setfacl -m u:1012:rx ssl

Copy your ssl certs to ssl directory.

    $ cp server.crt ssl
    $ cp server.key ssl
    $ chmod 600 ssl/server.crt
    $ chmod 600 ssl/server.key
    $ setfacl -m u:1012:r ssl/server.crt
    $ setfacl -m u:1012:r ssl/server.key
