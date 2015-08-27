#!/usr/bin/env ruby

class PostgresNode < Node

  class << self

    def create
      new({
        name: 'postgres'
      }, [], {
        from: 'archlinux'
      })
    end
  end

  def host_env
    {ADDR: dbaddr, PORT: dbport, USER: dbuser, PASS: dbpass,
     DB:   dbname, ADAP: dbadap, CONN: dbconn}
  end

  def dbaddr; raise 'subclass'                  end
  def dbport; '5432'                            end
  def dbuser; raise 'subclass'                  end
  def dbpass; @dbpass ||= SecureRandom.hex(800) end
  def dbname; raise 'subclass'                  end
  def dbadap; 'postgresql'                      end
  def dbconn
    "postgres://#{dbuser}:#{dbpass}@#{dbaddr}:#{dbport}/#{dbname}"
  end

  def pwd; Pathname.new('/var/lib') + name end

  def generate
    generate_header
    install_postgres
    configure_postgres
    generate_interface
  end

  def install_postgres
    run('pacman', '-Syy')
    run('pacman', '-S', '--noconfirm', 'postgresql')
  end

  def configure_postgres
    setup_unix_socket_directory
    as_user do
    end
  end

  def setup_unix_socket_directory
    dir = '/run/postgresql'
    run('mkdir', dir)
    run('chown', "#{name}:#{name}", dir)
  end

  def generate_interface
    cmd(pwd + entrypoint)
  end

  def entrypoint; 'entrypoint.sh' end

  def postgres_path; Pathname.new('postgres') end
  def data_path;     Pathname.new('data')     end

  def volume
    [[pwd, host_pwd + postgres_path]]
  end

  def volume_exclude
    [postgres_path]
  end

  def dependent_volume_setup?
    true
  end

  def volume_setup?
    (node_pwd + postgres_path).exist?
  end

  def setup_volume
    script('id -u')
    @data[:uid] = exec_container('getuser', Binds: [])[/\d+/]
    mkdir(node_pwd + postgres_path)
    File.open(node_pwd + postgres_path + entrypoint, 'w') do |f|
      start_server = "postgres -p #{dbport} -D #{pwd + data_path}"
      pg_hba     = pwd + data_path + 'pg_hba.conf'
      postgresql = pwd + data_path + 'postgresql.conf'
      f.puts('#!/bin/bash')
      f.puts("#{start_server} &")
      f.puts('pid="$!"')
      f.puts('sleep 3')
      f.puts("rol=`psql -tAc \"SELECT 1 FROM pg_roles WHERE rolname = '#{dbuser}'\"`")
      f.puts('if [ -z "$rol" ]; then')
      f.puts("  echo \"CREATE USER \\\"#{dbuser}\\\" WITH PASSWORD '$PASS'\" | psql")
      f.puts("  createdb -O #{dbuser.inspect} #{dbname.inspect}")
      f.puts('else')
      f.puts("  echo \"ALTER USER \\\"#{dbuser}\\\" WITH PASSWORD '$PASS'\" | psql")
      f.puts('fi')
      f.puts('kill -s TERM "$pid"')
      f.puts('wait "$pid"')
      f.puts("echo \"host all all 0.0.0.0/0 md5\" >> #{pg_hba}")
      f.puts("echo \"listen_addresses = '*'\" >> #{postgresql}")
      f.puts("exec #{start_server}")
    end
    `chmod +x #{node_pwd + postgres_path + entrypoint}`
    script('initdb',
           '--locale', 'en_US.UTF-8',
           '-E', 'UTF8',
           '-D', pwd + data_path)
    exec_container('setup')
  end

  def clean_volume
    (node_pwd + postgres_path + entrypoint).delete
    (node_pwd + postgres_path).children(false).each do |c|
      script('rm', '-r', pwd + c)
    end
    exec_container('clean')
    rmdir(node_pwd + postgres_path)
  end
end
