# Copyright (C) 2009-2014  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class FixSizeColumnTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    setup_database
  end

  class OperationTest < self
    def setup
      super
      setup_bookmarks_table
    end

    def setup_bookmarks_table
      @bookmarks_path = @tables_dir + "bookmarks"
      @bookmarks = Groonga::Array.create(:name => "Bookmarks",
                                         :path => @bookmarks_path.to_s)

      @n_viewed_column_path = @columns_dir + "n_viewed"
      @n_viewed = @bookmarks.define_column("n_viewed", "Int32",
                                           :path => @n_viewed_column_path.to_s)
    end

    def test_index?
      assert_not_predicate(@n_viewed, :index?)
    end

    def test_vector?
      assert_not_predicate(@n_viewed, :vector?)
    end

    def test_scalar?
      assert_predicate(@n_viewed, :scalar?)
    end

    def test_inspect
      assert_equal("#<Groonga::FixSizeColumn " +
                   "id: <#{@n_viewed.id}>, " +
                   "name: <Bookmarks.n_viewed>, " +
                   "path: <#{@n_viewed_column_path}>, " +
                   "domain: <Bookmarks>, " +
                   "range: <Int32>, " +
                   "flags: <KEY_INT>" +
                   ">",
                   @n_viewed.inspect)
    end

    def test_domain
      assert_equal(@bookmarks, @n_viewed.domain)
    end

    def test_table
      assert_equal(@bookmarks, @n_viewed.table)
    end

    class AssignTest < self
      def test_different_types
        @bookmarks.add(:n_viewed => "100")
        record = @bookmarks.add(:n_viewed => 101)
        assert_equal(101, record.n_viewed)
      end
    end
  end

  class TimeTest < self
    class IntegerTest < self
      def test_assign
        comments = Groonga::Array.create(:name => "Comments")
        comments.define_column("issued", "Time")
        issued = 1187430026
        record = comments.add(:issued => issued)
        assert_equal(Time.at(issued), record["issued"])
      end
    end

    class FloatTest < self
      def test_assign
        comments = Groonga::Array.create(:name => "Comments")
        comments.define_column("issued", "Time")
        issued = 1187430026.29
        record = comments.add(:issued => issued)
        assert_in_delta(issued, record["issued"].to_f, 0.001)
      end
    end

    class TimeTest < self
      def test_assign
        comments = Groonga::Array.create(:name => "Comments")
        comments.define_column("issued", "Time")
        issued_in_float = 1187430026.29
        issued = Time.at(issued_in_float)
        record = comments.add(:issued => issued)
        assert_in_delta(issued_in_float, record["issued"].to_f, 0.001)
      end
    end

    class StringTest < self
      def test_assign
        comments = Groonga::Array.create(:name => "Comments")
        comments.define_column("issued", "Time")
        record = comments.add(:issued => "2010/06/01 00:00:00")
        assert_equal(Time.new(2010, 6, 1, 0, 0, 0), record["issued"])
      end
    end
  end
end
