# typed: strict

require "date"

T.assert_type!(Date.today, Date)
T.assert_type!(Date.today + 1, Date)

class Test
  extend T::Sig

  sig {params(x: Date, y: DateTime).void}
  def test_date(x, y)
    T.reveal_type(x -  x) # error: Revealed type: `Rational`
    T.reveal_type(x - 10) # error: Revealed type: `Date`

    T.reveal_type(y - y) # error: Revealed type: `Rational`
    T.reveal_type(y - 10) # error: Revealed type: `DateTime`

    T.reveal_type(y - x) # error: Revealed type: `Rational`
    T.reveal_type(x - y) # error: Revealed type: `Rational`
  end
end
