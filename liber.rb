class Liber

  class << self

    def user; 'liber' end
    def dbaddr; 'pg' end
    def dbname; "#{user}_production" end
  end
end
