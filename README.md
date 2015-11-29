# Rock

A cookbook containing Rockerfiles and configurations for each container.

# General Service Usage

## Starting a Service

You need to generate the dockerfile and build the image of the *CONTAINER*.

    $ rocker rebuild rock CONTAINER

Then, setup the volume of the *CONTAINER*.

    $ rocker setup rock CONTAINER

Finally the *CONTAINER* is ready to run. You can run it by

    $ rocker restart rock CONTAINER

*CONTAINER* can be any folder name under the project, eg: `airweb_postgres`

## Stop a Service

For instance, delete *CONTAINER* container and **clean the volume**.

    $ rocker remove rock CONTAINER

# Special Service Usage

## NPM

### Start Service

    $ rocker setup npm_couchdb
    $ rocker restart npm_couchdb
    $ rocker setup npm
    $ rocker restart npm
    $ rocker destroy npm

### Stop Service

Delete *CONTAINER* container only

    $ rocker destroy rock npm_couchdb

Delete *CONTAINER* container and **clean the volume**.

    $ rocker remove rock npm_couchdb

## Git

### Upgrade Nodejs

    $ vim lib.rb
      def node_version; 'v4.2.2' end

    $ rocker destroy rock git
    $ rocker destroy rock CONTAINER
    $ rocker clean rock git CONTAINER
    $ rocker setup rock git CONTAINER
    $ rocker restart rock CONTAINER
    $ rocker restart rock git
    $ rocker rerun rock nginx
