# Rock

A cookbook containing Rockerfiles and configurations for each container.

# Starting a Service

For instance, setup the volume of `airweb_postgres` and run it.

    $ rocker setup rock airweb_postgres
    $ rocker restart rock airweb_postgres

# Stop a Service

For instance, delete `airweb_postgres` container and clean the volume.

    $ rocker remove rock airweb_postgres
