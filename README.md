# Rock

A cookbook containing Rockerfiles and configurations for each container.

# Usage

## Starting a Service

You need to generate the dockerfile and build the image of the *CONTAINER*.

    $ rocker rebuild rock <CONTAINER>

Then, setup the volume of the *CONTAINER*.

    $ rocker setup rock <CONTAINER>

Finally the *CONTAINER* is ready to run. You can run it by

    $ rocker restart rock <CONTAINER>

*CONTAINER* can be any folder name under the project, eg: `airweb_postgres`

## Stop a Service

For instance, delete *CONTAINER* container and clean the volume.

    $ rocker remove rock <CONTAINER>
