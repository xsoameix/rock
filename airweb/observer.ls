require! <[net fs]>
{spawn, exec} = require \child_process

info = -> console.log \info: it

class Observer

  run-server: ->
    @child = null
    @server = net.create-server (sock) ~>
      gets = (buf) ->
        line = /^[a-z]+\n/
        info = line.exec buf.to-string!
        if info == null
          return null
        matched = info[0]
        newbuf = buf.slice matched.length
        [matched, newbuf]

      buf = new Buffer(0)
      sock.on \data ~>
        buf := Buffer.concat([buf, it])
        while (result = gets(buf)) != null
          command = result[0]
          buf := result[1]
          switch command
          | "kill\n"
            @kill-app!
            sock.write "Kill the app ...\n" \utf8 ~>
          | "run\n"
            <~ @start-app false
            sock.write "Run the app ...\n" \utf8 ~>
          | "exit\n"
            sock.destroy!
    @start-app true, ->
    @server.listen process.env.OBSERVER_PORT, ->
      console.log "Listening on #{process.env.OBSERVER_PORT}"

  kill-app: ->
    if @child then that.kill!

  start-app: (first_time, cb) ->
    repo_path = process.env.REPO_PATH
    pivot = \package.json
    err, conf <~ fs.read-file "#repo_path/#pivot", \utf8
    if err
      if first_time then return
      console.log err
      return
    main = JSON.parse(conf)[\main]
    entry = "#repo_path/#main"
    err, stat <~ fs.stat entry
    if err
      console.log err
      return
    env <~ @get-env!
    @child = spawn \node [main] env: env, cwd: repo_path
    @child.stdout.on \data -> process.stdout.write it.to-string!
    @child.stderr.on \data -> process.stdout.write it.to-string!
    @child.on \error -> console.log \err: it
    @child.on \close -> console.log \info: "App exit code #it"
    cb!

  get-env: (cb) ->
    cb process.env{PATH, NODE_ENV, PORT, SSL_CRT, SSL_KEY} <<<
      PG_CONN: process.env.PG_ENV_CONN
      REDIS_HOST: process.env.REDIS_ENV_ADDR
      REDIS_PORT: process.env.REDIS_ENV_PORT
      SESSION_SECRET: process.env.SESSION_SECRET

observer = new Observer!
exit = ->
  observer.kill-app!
  process.exit!
process.on \SIGTERM exit
process.on \SIGINT exit
observer.run-server!
