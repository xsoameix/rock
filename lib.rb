module Compilers

  def setup_compiler_env
    run('mkdir', pwd + compiler_path)
    @bin = pwd + compiler_path + 'bin'
    env('PATH', "#{@bin}:$PATH")
    add_config_json('PATH.json', [@bin])
  end

  def setup_npm_prefix
    run('npm', 'config', 'set', 'prefix', pwd + compiler_path)
  end

  def install_livescript
    @lsc = @bin + 'lsc'
    run('npm', 'install', '-g', 'livescript', 'node-gyp')
  end

  def install_nitrogen
    @nitrogen = @bin + 'nitrogen'
    run('pushd', pwd + compiler_path + 'lib')
    run('git', 'clone', 'https://github.com/xsoameix/nitrogen')
    run('pushd', pwd + compiler_path + 'lib/nitrogen')
    run('cmake', "-DCMAKE_INSTALL_PREFIX:PATH=#{pwd + compiler_path}", '.')
    run('make')
    run('make install')
    run('popd')
    run('rm', '-r', pwd + compiler_path + 'lib/nitrogen')
    run('popd')
  end

  def compiler_path; Pathname.new('compiler') end
end

module Observer

  def configure_observer
    FileUtils.cp(node_pwd.dirname + observer_path, node_pwd + observer_path)
    add_config_file(pwd + observer_path, observer_path)
    run('gcc', '-o', pwd + observer_path.basename(observer_path.extname),
        pwd + observer_path);
  end

  def observer_path; Pathname.new('observer.c') end

  def gitignore
    [observer_path]
  end
end
