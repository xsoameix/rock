# Setup SSL Directory

Assume nginx's uid is 1021. Make sure that nginx can access your certs.

    $ mkdir certs
    $ chmod 700 certs
    $ setfacl -m u:1021:rx certs

Copy your ssl certs to certs directory.

    $ cp airweb.crt certs
    $ cp airweb.key certs
    $ chmod 600 certs/airweb.crt
    $ chmod 600 certs/airweb.key
    $ setfacl -m u:1021:r certs/airweb.crt
    $ setfacl -m u:1021:r certs/airweb.key

Do same thing with error.crt and error.key
