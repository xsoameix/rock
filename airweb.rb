class Airweb

  class << self

    def user; 'airweb' end
    def dbaddr; 'pg' end
    def dbname; "#{user}_production" end
    def session_addr; 'redis' end
    def m758_addr; 'm758' end
    def mpan_addr; 'mpan' end
  end
end
