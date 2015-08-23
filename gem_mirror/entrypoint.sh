#!/bin/bash

geminabox_pid=0
clean() {
  if [ $geminabox_pid -ne 0 ]; then
    kill -s SIGTERM "$geminabox_pid"
    wait "$geminabox_pid"
  fi
  ps -ef
  exit 0
}
trap 'kill ${!}; clean' SIGTERM
export PATH=$(ruby -e 'print Gem.user_dir')/bin:$PATH
rackup &
geminabox_pid="$!"
while true; do
  gem mirror & wait ${!}
done
