class Airweb

  class << self

    def user; 'airweb' end
    def dbaddr; 'pg' end
    def dbname; "#{user}_production" end
    def session_addr; 'redis' end
    def sender_addr; 'sender' end
  end
end
