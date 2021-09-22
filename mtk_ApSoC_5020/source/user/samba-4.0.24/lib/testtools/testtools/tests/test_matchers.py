# Copyright (c) 2008-2011 testtools developers. See LICENSE for details.

"""Tests for matchers."""

import doctest
import re
import os
import shutil
import sys
import tarfile
import tempfile

from testtools import (
    Matcher, # check that Matcher is exposed at the top level for docs.
    skipIf,
    TestCase,
    )
from testtools.compat import (
    StringIO,
    str_is_unicode,
    text_repr,
    _b,
    _u,
    )
from testtools.matchers import (
    AfterPreprocessing,
    AllMatch,
    Annotate,
    AnnotatedMismatch,
    _BinaryMismatch,
    Contains,
    DirContains,
    DirExists,
    DocTestMatches,
    DoesNotEndWith,
    DoesNotStartWith,
    EndsWith,
    Equals,
    FileContains,
    FileExists,
    HasPermissions,
    KeysEqual,
    Is,
    IsInstance,
    LessThan,
    GreaterThan,
    MatchesAny,
    MatchesAll,
    MatchesException,
    MatchesListwise,
    MatchesPredicate,
    MatchesRegex,
    MatchesSetwise,
    MatchesStructure,
    Mismatch,
    MismatchDecorator,
    MismatchError,
    Not,
    NotEquals,
    PathExists,
    Raises,
    raises,
    SamePath,
    StartsWith,
    TarballContains,
    )
from testtools.tests.helpers import FullStackRunTest

# Silence pyflakes.
Matcher


class TestMismatch(TestCase):

    run_tests_with = FullStackRunTest

    def test_constructor_arguments(self):
        mismatch = Mismatch("some description", {'detail': "things"})
        self.assertEqual("some description", mismatch.describe())
        self.assertEqual({'detail': "things"}, mismatch.get_details())

    def test_constructor_no_arguments(self):
        mismatch = Mismatch()
        self.assertThat(mismatch.describe,
            Raises(MatchesException(NotImplementedError)))
        self.assertEqual({}, mismatch.get_details())


class TestMismatchError(TestCase):

    def test_is_assertion_error(self):
        # MismatchError is an AssertionError, so that most of the time, it
        # looks like a test failure, rather than an error.
        def raise_mismatch_error():
            raise MismatchError(2, Equals(3), Equals(3).match(2))
        self.assertRaises(AssertionError, raise_mismatch_error)

    def test_default_description_is_mismatch(self):
        mismatch = Equals(3).match(2)
        e = MismatchError(2, Equals(3), mismatch)
        self.assertEqual(mismatch.describe(), str(e))

    def test_default_description_unicode(self):
        matchee = _u('\xa7')
        matcher = Equals(_u('a'))
        mismatch = matcher.match(matchee)
        e = MismatchError(matchee, matcher, mismatch)
        self.assertEqual(mismatch.describe(), str(e))

    def test_verbose_description(self):
        matchee = 2
        matcher = Equals(3)
        mismatch = matcher.match(2)
        e = MismatchError(matchee, matcher, mismatch, True)
        expected = (
            'Match failed. Matchee: %r\n'
            'Matcher: %s\n'
            'Difference: %s\n' % (
                matchee,
                matcher,
                matcher.match(matchee).describe(),
                ))
        self.assertEqual(expected, str(e))

    def test_verbose_unicode(self):
        # When assertThat is given matchees or matchers that contain non-ASCII
        # unicode strings, we can still provide a meaningful error.
        matchee = _u('\xa7')
        matcher = Equals(_u('a'))
        mismatch = matcher.match(matchee)
        expected = (
            'Match failed. Matchee: %s\n'
            'Matcher: %s\n'
            'Difference: %s\n' % (
                text_repr(matchee),
                matcher,
                mismatch.describe(),
                ))
        e = MismatchError(matchee, matcher, mismatch, True)
        if str_is_unicode:
            actual = str(e)
        else:
            actual = unicode(e)
            # Using str() should still work, and return ascii only
            self.assertEqual(
                expected.replace(matchee, matchee.encode("unicode-escape")),
                str(e).decode("ascii"))
        self.assertEqual(expected, actual)


class Test_BinaryMismatch(TestCase):
    """Mismatches from binary comparisons need useful describe output"""

    _long_string = "This is a longish multiline non-ascii string\n\xa7"
    _long_b = _b(_long_string)
    _long_u = _u(_long_string)

    def test_short_objects(self):
        o1, o2 = object(), object()
        mismatch = _BinaryMismatch(o1, "!~", o2)
        self.assertEqual(mismatch.describe(), "%r !~ %r" % (o1, o2))

    def test_short_mixed_strings(self):
        b, u = _b("\xa7"), _u("\xa7")
        mismatch = _BinaryMismatch(b, "!~", u)
        self.assertEqual(mismatch.describe(), "%r !~ %r" % (b, u))

    def test_long_bytes(self):
        one_line_b = self._long_b.replace(_b("\n"), _b(" "))
        mismatch = _BinaryMismatch(one_line_b, "!~", self._long_b)
        self.assertEqual(mismatch.describe(),
            "%s:\nreference = %s\nactual    = %s\n" % ("!~",
                text_repr(one_line_b),
                text_repr(self._long_b, multiline=True)))

    def test_long_unicode(self):
        one_line_u = self._long_u.replace("\n", " ")
        mismatch = _BinaryMismatch(one_line_u, "!~", self._long_u)
        self.assertEqual(mismatch.describe(),
            "%s:\nreference = %s\nactual    = %s\n" % ("!~",
                text_repr(one_line_u),
                text_repr(self._long_u, multiline=True)))

    def test_long_mixed_strings(self):
        mismatch = _BinaryMismatch(self._long_b, "!~", self._long_u)
        self.assertEqual(mismatch.describe(),
            "%s:\nreference = %s\nactual    = %s\n" % ("!~",
                text_repr(self._long_b, multiline=True),
                text_repr(self._long_u, multiline=True)))

    def test_long_bytes_and_object(self):
        obj = object()
        mismatch = _BinaryMismatch(self._long_b, "!~", obj)
        self.assertEqual(mismatch.describe(),
            "%s:\nreference = %s\nactual    = %s\n" % ("!~",
                text_repr(self._long_b, multiline=True),
                repr(obj)))

    def test_long_unicode_and_object(self):
        obj = object()
        mismatch = _BinaryMismatch(self._long_u, "!~", obj)
        self.assertEqual(mismatch.describe(),
            "%s:\nreference = %s\nactual    = %s\n" % ("!~",
                text_repr(self._long_u, multiline=True),
                repr(obj)))


class TestMatchersInterface(object):

    run_tests_with = FullStackRunTest

    def test_matches_match(self):
        matcher = self.matches_matcher
        matches = self.matches_matches
        mismatches = self.matches_mismatches
        for candidate in matches:
            self.assertEqual(None, matcher.match(candidate))
        for candidate in mismatches:
            mismatch = matcher.match(candidate)
            self.assertNotEqual(None, mismatch)
            self.assertNotEqual(None, getattr(mismatch, 'describe', None))

    def test__str__(self):
        # [(expected, object to __str__)].
        examples = self.str_examples
        for expected, matcher in examples:
            self.assertThat(matcher, DocTestMatches(expected))

    def test_describe_difference(self):
        # [(expected, matchee, matcher), ...]
        examples = self.describe_examples
        for difference, matchee, matcher in examples:
            mismatch = matcher.match(matchee)
            self.assertEqual(difference, mismatch.describe())

    def test_mismatch_details(self):
        # The mismatch object must provide get_details, which must return a
        # dictionary mapping names to Content objects.
        examples = self.describe_examples
        for difference, matchee, matcher in examples:
            mismatch = matcher.match(matchee)
            details = mismatch.get_details()
            self.assertEqual(dict(details), details)


class TestDocTestMatchesInterface(TestCase, TestMatchersInterface):

    matches_matcher = DocTestMatches("Ran 1 test in ...s", doctest.ELLIPSIS)
    matches_matches = ["Ran 1 test in 0.000s", "Ran 1 test in 1.234s"]
    matches_mismatches = ["Ran 1 tests in 0.000s", "Ran 2 test in 0.000s"]

    str_examples = [("DocTestMatches('Ran 1 test in ...s\\n')",
        DocTestMatches("Ran 1 test in ...s")),
        ("DocTestMatches('foo\\n', flags=8)", DocTestMatches("foo", flags=8)),
        ]

    describe_examples = [('Expected:\n    Ran 1 tests in ...s\nGot:\n'
        '    Ran 1 test in 0.123s\n', "Ran 1 test in 0.123s",
        DocTestMatches("Ran 1 tests in ...s", doctest.ELLIPSIS))]


class TestDocTestMatchesInterfaceUnicode(TestCase, TestMatchersInterface):

    matches_matcher = DocTestMatches(_u("\xa7..."), doctest.ELLIPSIS)
    matches_matches = [_u("\xa7"), _u("\xa7 more\n")]
    matches_mismatches = ["\\xa7", _u("more \xa7"), _u("\n\xa7")]

    str_examples = [("DocTestMatches(%r)" % (_u("\xa7\n"),),
        DocTestMatches(_u("\xa7"))),
        ]

    describe_examples = [(
        _u("Expected:\n    \xa7\nGot:\n    a\n"),
        "a",
        DocTestMatches(_u("\xa7"), doctest.ELLIPSIS))]


class TestDocTestMatchesSpecific(TestCase):

    run_tests_with = FullStackRunTest

    def test___init__simple(self):
        matcher = DocTestMatches("foo")
        self.assertEqual("foo\n", matcher.want)

    def test___init__flags(self):
        matcher = DocTestMatches("bar\n", doctest.ELLIPSIS)
        self.assertEqual("bar\n", matcher.want)
        self.assertEqual(doctest.ELLIPSIS, matcher.flags)

    def test_describe_non_ascii_bytes(self):
        """Even with bytestrings, the mismatch should be coercible to unicode

        DocTestMatches is intended for text, but the Python 2 str type also
        permits arbitrary binary inputs. This is a slightly bogus thing to do,
        and under Python 3 using bytes objects will reasonably raise an error.
        """
        header = _b("\x89PNG\r\n\x1a\n...")
        if str_is_unicode:
            self.assertRaises(TypeError,
                DocTestMatches, header, doctest.ELLIPSIS)
            return
        matcher = DocTestMatches(header, doctest.ELLIPSIS)
        mismatch = matcher.match(_b("GIF89a\1\0\1\0\0\0\0;"))
        # Must be treatable as unicode text, the exact output matters less
        self.assertTrue(unicode(mismatch.describe()))


class TestEqualsInterface(TestCase, TestMatchersInterface):

    matches_matcher = Equals(1)
    matches_matches = [1]
    matches_mismatches = [2]

    str_examples = [("Equals(1)", Equals(1)), ("Equals('1')", Equals('1'))]

    describe_examples = [("1 != 2", 2, Equals(1))]


class TestNotEqualsInterface(TestCase, TestMatchersInterface):

    matches_matcher = NotEquals(1)
    matches_matches = [2]
    matches_mismatches = [1]

    str_examples = [
        ("NotEquals(1)", NotEquals(1)), ("NotEquals('1')", NotEquals('1'))]

    describe_examples = [("1 == 1", 1, NotEquals(1))]


class TestIsInterface(TestCase, TestMatchersInterface):

    foo = object()
    bar = object()

    matches_matcher = Is(foo)
    matches_matches = [foo]
    matches_mismatches = [bar, 1]

    str_examples = [("Is(2)", Is(2))]

    describe_examples = [("1 is not 2", 2, Is(1))]


class TestIsInstanceInterface(TestCase, TestMatchersInterface):

    class Foo:pass

    matches_matcher = IsInstance(Foo)
    matches_matches = [Foo()]
    matches_mismatches = [object(), 1, Foo]

    str_examples = [
            ("IsInstance(str)", IsInstance(str)),
            ("IsInstance(str, int)", IsInstance(str, int)),
            ]

    describe_examples = [
            ("'foo' is not an instance of int", 'foo', IsInstance(int)),
            ("'foo' is not an instance of any of (int, type)", 'foo',
             IsInstance(int, type)),
            ]


class TestLessThanInterface(TestCase, TestMatchersInterface):

    matches_matcher = LessThan(4)
    matches_matches = [-5, 3]
    matches_mismatches = [4, 5, 5000]

    str_examples = [
        ("LessThan(12)", LessThan(12)),
        ]

    describe_examples = [
        ('4 is not > 5', 5, LessThan(4)),
        ('4 is not > 4', 4, LessThan(4)),
        ]


class TestGreaterThanInterface(TestCase, TestMatchersInterface):

    matches_matcher = GreaterThan(4)
    matches_matches = [5, 8]
    matches_mismatches = [-2, 0, 4]

    str_examples = [
        ("GreaterThan(12)", GreaterThan(12)),
        ]

    describe_examples = [
        ('5 is not < 4', 4, GreaterThan(5)),
        ('4 is not < 4', 4, GreaterThan(4)),
        ]


class TestContainsInterface(TestCase, TestMatchersInterface):

    matches_matcher = Contains('foo')
    matches_matches = ['foo', 'afoo', 'fooa']
    matches_mismatches = ['f', 'fo', 'oo', 'faoo', 'foao']

    str_examples = [
        ("Contains(1)", Contains(1)),
        ("Contains('foo')", Contains('foo')),
        ]

    describe_examples = [("1 not in 2", 2, Contains(1))]


def make_error(type, *args, **kwargs):
    try:
        raise type(*args, **kwargs)
    except type:
        return sys.exc_info()


class TestMatchesExceptionInstanceInterface(TestCase, TestMatchersInterface):

    matches_matcher = MatchesException(ValueError("foo"))
    error_foo = make_error(ValueError, 'foo')
    error_bar = make_error(ValueError, 'bar')
    error_base_foo = make_error(Exception, 'foo')
    matches_matches = [error_foo]
    matches_mismatches = [error_bar, error_base_foo]

    str_examples = [
        ("MatchesException(Exception('foo',))",
         MatchesException(Exception('foo')))
        ]
    describe_examples = [
        ("%r is not a %r" % (Exception, ValueError),
         error_base_foo,
         MatchesException(ValueError("foo"))),
        ("ValueError('bar',) has different arguments to ValueError('foo',).",
         error_bar,
         MatchesException(ValueError("foo"))),
        ]


class TestMatchesExceptionTypeInterface(TestCase, TestMatchersInterface):

    matches_matcher = MatchesException(ValueError)
    error_foo = make_error(ValueError, 'foo')
    error_sub = make_error(UnicodeError, 'bar')
    error_base_foo = make_error(Exception, 'foo')
    matches_matches = [error_foo, error_sub]
    matches_mismatches = [error_base_foo]

    str_examples = [
        ("MatchesException(%r)" % Exception,
         MatchesException(Exception))
        ]
    describe_examples = [
        ("%r is not a %r" % (Exception, ValueError),
         error_base_foo,
         MatchesException(ValueError)),
        ]


class TestMatchesExceptionTypeReInterface(TestCase, TestMatchersInterface):

    matches_matcher = MatchesException(ValueError, 'fo.')
    error_foo = make_error(ValueError, 'foo')
    error_sub = make_error(UnicodeError, 'foo')
    error_bar = make_error(ValueError, 'bar')
    matches_matches = [error_foo, error_sub]
    matches_mismatches = [error_bar]

    str_examples = [
        ("MatchesException(%r)" % Exception,
         MatchesException(Exception, 'fo.'))
        ]
    describe_examples = [
        ("'bar' does not match /fo./",
         error_bar, MatchesException(ValueError, "fo.")),
        ]


class TestMatchesExceptionTypeMatcherInterface(TestCase, TestMatchersInterface):

    matches_matcher = MatchesException(
        ValueError, AfterPreprocessing(str, Equals('foo')))
    error_foo = make_error(ValueError, 'foo')
    error_sub = make_error(UnicodeError, 'foo')
    error_bar = make_error(ValueError, 'bar')
    matches_matches = [error_foo, error_sub]
    matches_mismatches = [error_bar]

    str_examples = [
        ("MatchesException(%r)" % Exception,
         MatchesException(Exception, Equals('foo')))
        ]
    describe_examples = [
        ("5 != %r" % (error_bar[1],),
         error_bar, MatchesException(ValueError, Equals(5))),
        ]


class TestNotInterface(TestCase, TestMatchersInterface):

    matches_matcher = Not(Equals(1))
    matches_matches = [2]
    matches_mismatches = [1]

    str_examples = [
        ("Not(Equals(1))", Not(Equals(1))),
        ("Not(Equals('1'))", Not(Equals('1')))]

    describe_examples = [('1 matches Equals(1)', 1, Not(Equals(1)))]


class TestMatchersAnyInterface(TestCase, TestMatchersInterface):

    matches_matcher = MatchesAny(DocTestMatches("1"), DocTestMatches("2"))
    matches_matches = ["1", "2"]
    matches_mismatches = ["3"]

    str_examples = [(
        "MatchesAny(DocTestMatches('1\\n'), DocTestMatches('2\\n'))",
        MatchesAny(DocTestMatches("1"), DocTestMatches("2"))),
        ]

    describe_examples = [("""Differences: [
Expected:
    1
Got:
    3

Expected:
    2
Got:
    3

]""",
        "3", MatchesAny(DocTestMatches("1"), DocTestMatches("2")))]


class TestMatchesAllInterface(TestCase, TestMatchersInterface):

    matches_matcher = MatchesAll(NotEquals(1), NotEquals(2))
    matches_matches = [3, 4]
    matches_mismatches = [1, 2]

    str_examples = [
        ("MatchesAll(NotEquals(1), NotEquals(2))",
         MatchesAll(NotEquals(1), NotEquals(2)))]

    describe_examples = [
        ("""Differences: [
1 == 1
]""",
         1, MatchesAll(NotEquals(1), NotEquals(2))),
        ("1 == 1", 1,
         MatchesAll(NotEquals(2), NotEquals(1), Equals(3), first_only=True)),
        ]


class TestKeysEqual(TestCase, TestMatchersInterface):

    matches_matcher = KeysEqual('foo', 'bar')
    matches_matches = [
        {'foo': 0, 'bar': 1},
        ]
    matches_mismatches = [
        {},
        {'foo': 0},
        {'bar': 1},
        {'foo': 0, 'bar': 1, 'baz': 2},
        {'a': None, 'b': None, 'c': None},
        ]

    str_examples = [
        ("KeysEqual('foo', 'bar')", KeysEqual('foo', 'bar')),
        ]

    describe_examples = [
        ("['bar', 'foo'] does not match {'baz': 2, 'foo': 0, 'bar': 1}: "
         "Keys not equal",
         {'foo': 0, 'bar': 1, 'baz': 2}, KeysEqual('foo', 'bar')),
        ]


class TestAnnotate(TestCase, TestMatchersInterface):

    matches_matcher = Annotate("foo", Equals(1))
    matches_matches = [1]
    matches_mismatches = [2]

    str_examples = [
        ("Annotate('foo', Equals(1))", Annotate("foo", Equals(1)))]

    describe_examples = [("1 != 2: foo", 2, Annotate('foo', Equals(1)))]

    def test_if_message_no_message(self):
        # Annotate.if_message returns the given matcher if there is no
        # message.
        matcher = Equals(1)
        not_annotated = Annotate.if_message('', matcher)
        self.assertIs(matcher, not_annotated)

    def test_if_message_given_message(self):
        # Annotate.if_message returns an annotated version of the matcher if a
        # message is provided.
        matcher = Equals(1)
        expected = Annotate('foo', matcher)
        annotated = Annotate.if_message('foo', matcher)
        self.assertThat(
            annotated,
            MatchesStructure.fromExample(expected, 'annotation', 'matcher'))


class TestAnnotatedMismatch(TestCase):

    run_tests_with = FullStackRunTest

    def test_forwards_details(self):
        x = Mismatch('description', {'foo': 'bar'})
        annotated = AnnotatedMismatch("annotation", x)
        self.assertEqual(x.get_details(), annotated.get_details())


class TestRaisesInterface(TestCase, TestMatchersInterface):

    matches_matcher = Raises()
    def boom():
        raise Exception('foo')
    matches_matches = [boom]
    matches_mismatches = [lambda:None]

    # Tricky to get function objects to render constantly, and the interfaces
    # helper uses assertEqual rather than (for instance) DocTestMatches.
    str_examples = []

    describe_examples = []


class TestRaisesExceptionMatcherInterface(TestCase, TestMatchersInterface):

    matches_matcher = Raises(
        exception_matcher=MatchesException(Exception('foo')))
    def boom_bar():
        raise Exception('bar')
    def boom_foo():
        raise Exception('foo')
    matches_matches = [boom_foo]
    matches_mismatches = [lambda:None, boom_bar]

    # Tricky to get function objects to render constantly, and the interfaces
    # helper uses assertEqual rather than (for instance) DocTestMatches.
    str_examples = []

    describe_examples = []


class TestRaisesBaseTypes(TestCase):

    run_tests_with = FullStackRunTest

    def raiser(self):
        raise KeyboardInterrupt('foo')

    def test_KeyboardInterrupt_matched(self):
        # When KeyboardInterrupt is matched, it is swallowed.
        matcher = Raises(MatchesException(KeyboardInterrupt))
        self.assertThat(self.raiser, matcher)

    def test_KeyboardInterrupt_propogates(self):
        # The default 'it raised' propogates KeyboardInterrupt.
        match_keyb = Raises(MatchesException(KeyboardInterrupt))
        def raise_keyb_from_match():
            matcher = Raises()
            matcher.match(self.raiser)
        self.assertThat(raise_keyb_from_match, match_keyb)

    def test_KeyboardInterrupt_match_Exception_propogates(self):
        # If the raised exception isn't matched, and it is not a subclass of
        # Exception, it is propogated.
        match_keyb = Raises(MatchesException(KeyboardInterrupt))
        def raise_keyb_from_match():
            if sys.version_info > (2, 5):
                matcher = Raises(MatchesException(Exception))
            else:
                # On Python 2.4 KeyboardInterrupt is a StandardError subclass
                # but should propogate from less generic exception matchers
                matcher = Raises(MatchesException(EnvironmentError))
            matcher.match(self.raiser)
        self.assertThat(raise_keyb_from_match, match_keyb)


class TestRaisesConvenience(TestCase):

    run_tests_with = FullStackRunTest

    def test_exc_type(self):
        self.assertThat(lambda: 1/0, raises(ZeroDivisionError))

    def test_exc_value(self):
        e = RuntimeError("You lose!")
        def raiser():
            raise e
        self.assertThat(raiser, raises(e))


class DoesNotStartWithTests(TestCase):

    run_tests_with = FullStackRunTest

    def test_describe(self):
        mismatch = DoesNotStartWith("fo", "bo")
        self.assertEqual("'fo' does not start with 'bo'.", mismatch.describe())

    def test_describe_non_ascii_unicode(self):
        string = _u("A\xA7")
        suffix = _u("B\xA7")
        mismatch = DoesNotStartWith(string, suffix)
        self.assertEqual("%s does not start with %s." % (
            text_repr(string), text_repr(suffix)),
            mismatch.describe())

    def test_describe_non_ascii_bytes(self):
        string = _b("A\xA7")
        suffix = _b("B\xA7")
        mismatch = DoesNotStartWith(string, suffix)
        self.assertEqual("%r does not start with %r." % (string, suffix),
            mismatch.describe())


class StartsWithTests(TestCase):

    run_tests_with = FullStackRunTest

    def test_str(self):
        matcher = StartsWith("bar")
        self.assertEqual("StartsWith('bar')", str(matcher))

    def test_str_with_bytes(self):
        b = _b("\xA7")
        matcher = StartsWith(b)
        self.assertEqual("StartsWith(%r)" % (b,), str(matcher))

    def test_str_with_unicode(self):
        u = _u("\xA7")
        matcher = StartsWith(u)
        self.assertEqual("StartsWith(%r)" % (u,), str(matcher))

    def test_match(self):
        matcher = StartsWith("bar")
        self.assertIs(None, matcher.match("barf"))

    def test_mismatch_returns_does_not_start_with(self):
        matcher = StartsWith("bar")
        self.assertIsInstance(matcher.match("foo"), DoesNotStartWith)

    def test_mismatch_sets_matchee(self):
        matcher = StartsWith("bar")
        mismatch = matcher.match("foo")
        self.assertEqual("foo", mismatch.matchee)

    def test_mismatch_sets_expected(self):
        matcher = StartsWith("bar")
        mismatch = matcher.match("foo")
        self.assertEqual("bar", mismatch.expected)


class DoesNotEndWithTests(TestCase):

    run_tests_with = FullStackRunTest

    def test_describe(self):
        mismatch = DoesNotEndWith("fo", "bo")
        self.assertEqual("'fo' does not end with 'bo'.", mismatch.describe())

    def test_describe_non_ascii_unicode(self):
        string = _u("A\xA7")
        suffix = _u("B\xA7")
        mismatch = DoesNotEndWith(string, suffix)
        self.assertEqual("%s does not end with %s." % (
            text_repr(string), text_repr(suffix)),
            mismatch.describe())

    def test_describe_non_ascii_bytes(self):
        string = _b("A\xA7")
        suffix = _b("B\xA7")
        mismatch = DoesNotEndWith(string, suffix)
        self.assertEqual("%r does not end with %r." % (string, suffix),
            mismatch.describe())


class EndsWithTests(TestCase):

    run_tests_with = FullStackRunTest

    def test_str(self):
        matcher = EndsWith("bar")
        self.assertEqual("EndsWith('bar')", str(matcher))

    def test_str_with_bytes(self):
        b = _b("\xA7")
        matcher = EndsWith(b)
        self.assertEqual("EndsWith(%r)" % (b,), str(matcher))

    def test_str_with_unicode(self):
        u = _u("\xA7")
        matcher = EndsWith(u)
        self.assertEqual("EndsWith(%r)" % (u,), str(matcher))

    def test_match(self):
        matcher = EndsWith("arf")
        self.assertIs(None, matcher.match("barf"))

    def test_mismatch_returns_does_not_end_with(self):
        matcher = EndsWith("bar")
        self.assertIsInstance(matcher.match("foo"), DoesNotEndWith)

    def test_mismatch_sets_matchee(self):
        matcher = EndsWith("bar")
        mismatch = matcher.match("foo")
        self.assertEqual("foo", mismatch.matchee)

    def test_mismatch_sets_expected(self):
        matcher = EndsWith("bar")
        mismatch = matcher.match("foo")
        self.assertEqual("bar", mismatch.expected)


def run_doctest(obj, name):
    p = doctest.DocTestParser()
    t = p.get_doctest(
        obj.__doc__, sys.modules[obj.__module__].__dict__, name, '', 0)
    r = doctest.DocTestRunner()
    output = StringIO()
    r.run(t, out=output.write)
    return r.failures, output.getvalue()


class TestMatchesListwise(TestCase):

    run_tests_with = FullStackRunTest

    def test_docstring(self):
        failure_count, output = run_doctest(
            MatchesListwise, "MatchesListwise")
        if failure_count:
            self.fail("Doctest failed with %s" % output)


class TestMatchesStructure(TestCase, TestMatchersInterface):

    class SimpleClass:
        def __init__(self, x, y):
            self.x = x
            self.y = y

    matches_matcher = MatchesStructure(x=Equals(1), y=Equals(2))
    matches_matches = [SimpleClass(1, 2)]
    matches_mismatches = [
        SimpleClass(2, 2),
        SimpleClass(1, 1),
        SimpleClass(3, 3),
        ]

    str_examples = [
        ("MatchesStructure(x=Equals(1))", MatchesStructure(x=Equals(1))),
        ("MatchesStructure(y=Equals(2))", MatchesStructure(y=Equals(2))),
        ("MatchesStructure(x=Equals(1), y=Equals(2))",
         MatchesStructure(x=Equals(1), y=Equals(2))),
        ]

    describe_examples = [
        ("""\
Differences: [
3 != 1: x
]""", SimpleClass(1, 2), MatchesStructure(x=Equals(3), y=Equals(2))),
        ("""\
Differences: [
3 != 2: y
]""", SimpleClass(1, 2), MatchesStructure(x=Equals(1), y=Equals(3))),
        ("""\
Differences: [
0 != 1: x
0 != 2: y
]""", SimpleClass(1, 2), MatchesStructure(x=Equals(0), y=Equals(0))),
        ]

    def test_fromExample(self):
        self.assertThat(
            self.SimpleClass(1, 2),
            MatchesStructure.fromExample(self.SimpleClass(1, 3), 'x'))

    def test_byEquality(self):
        self.assertThat(
            self.SimpleClass(1, 2),
            MatchesStructure.byEquality(x=1))

    def test_withStructure(self):
        self.assertThat(
            self.SimpleClass(1, 2),
            MatchesStructure.byMatcher(LessThan, x=2))

    def test_update(self):
        self.assertThat(
            self.SimpleClass(1, 2),
            MatchesStructure(x=NotEquals(1)).update(x=Equals(1)))

    def test_update_none(self):
        self.assertThat(
            self.SimpleClass(1, 2),
            MatchesStructure(x=Equals(1), z=NotEquals(42)).update(
                z=None))


class TestMatchesRegex(TestCase, TestMatchersInterface):

    matches_matcher = MatchesRegex('a|b')
    matches_matches = ['a', 'b']
    matches_mismatches = ['c']

    str_examples = [
        ("MatchesRegex('a|b')", MatchesRegex('a|b')),
        ("MatchesRegex('a|b', re.M)", MatchesRegex('a|b', re.M)),
        ("MatchesRegex('a|b', re.I|re.M)", MatchesRegex('a|b', re.I|re.M)),
        ("MatchesRegex(%r)" % (_b("\xA7"),), MatchesRegex(_b("\xA7"))),
        ("MatchesRegex(%r)" % (_u("\xA7"),), MatchesRegex(_u("\xA7"))),
        ]

    describe_examples = [
        ("'c' does not match /a|b/", 'c', MatchesRegex('a|b')),
        ("'c' does not match /a\d/", 'c', MatchesRegex(r'a\d')),
        ("%r does not match /\\s+\\xa7/" % (_b('c'),),
            _b('c'), MatchesRegex(_b("\\s+\xA7"))),
        ("%r does not match /\\s+\\xa7/" % (_u('c'),),
            _u('c'), MatchesRegex(_u("\\s+\xA7"))),
        ]


class TestMatchesSetwise(TestCase):

    run_tests_with = FullStackRunTest

    def assertMismatchWithDescriptionMatching(self, value, matcher,
                                              description_matcher):
        mismatch = matcher.match(value)
        if mismatch is None:
            self.fail("%s matched %s" % (matcher, value))
        actual_description = mismatch.describe()
        self.assertThat(
            actual_description,
            Annotate(
                "%s matching %s" % (matcher, value),
                description_matcher))

    def test_matches(self):
        self.assertIs(
            None, MatchesSetwise(Equals(1), Equals(2)).match([2, 1]))

    def test_mismatches(self):
        self.assertMismatchWithDescriptionMatching(
            [2, 3], MatchesSetwise(Equals(1), Equals(2)),
            MatchesRegex('.*There was 1 mismatch$', re.S))

    def test_too_many_matchers(self):
        self.assertMismatchWithDescriptionMatching(
            [2, 3], MatchesSetwise(Equals(1), Equals(2), Equals(3)),
            Equals('There was 1 matcher left over: Equals(1)'))

    def test_too_many_values(self):
        self.assertMismatchWithDescriptionMatching(
            [1, 2, 3], MatchesSetwise(Equals(1), Equals(2)),
            Equals('There was 1 value left over: [3]'))

    def test_two_too_many_matchers(self):
        self.assertMismatchWithDescriptionMatching(
            [3], MatchesSetwise(Equals(1), Equals(2), Equals(3)),
            MatchesRegex(
                'There were 2 matchers left over: Equals\([12]\), '
                'Equals\([12]\)'))

    def test_two_too_many_values(self):
        self.assertMismatchWithDescriptionMatching(
            [1, 2, 3, 4], MatchesSetwise(Equals(1), Equals(2)),
            MatchesRegex(
                'There were 2 values left over: \[[34], [34]\]'))

    def test_mismatch_and_too_many_matchers(self):
        self.assertMismatchWithDescriptionMatching(
            [2, 3], MatchesSetwise(Equals(0), Equals(1), Equals(2)),
            MatchesRegex(
                '.*There was 1 mismatch and 1 extra matcher: Equals\([01]\)',
                re.S))

    def test_mismatch_and_too_many_values(self):
        self.assertMismatchWithDescriptionMatching(
            [2, 3, 4], MatchesSetwise(Equals(1), Equals(2)),
            MatchesRegex(
                '.*There was 1 mismatch and 1 extra value: \[[34]\]',
                re.S))

    def test_mismatch_and_two_too_many_matchers(self):
        self.assertMismatchWithDescriptionMatching(
            [3, 4], MatchesSetwise(
                Equals(0), Equals(1), Equals(2), Equals(3)),
            MatchesRegex(
                '.*There was 1 mismatch and 2 extra matchers: '
                'Equals\([012]\), Equals\([012]\)', re.S))

    def test_mismatch_and_two_too_many_values(self):
        self.assertMismatchWithDescriptionMatching(
            [2, 3, 4, 5], MatchesSetwise(Equals(1), Equals(2)),
            MatchesRegex(
                '.*There was 1 mismatch and 2 extra values: \[[145], [145]\]',
                re.S))


class TestAfterPreprocessing(TestCase, TestMatchersInterface):

    def parity(x):
        return x % 2

    matches_matcher = AfterPreprocessing(parity, Equals(1))
    matches_matches = [3, 5]
    matches_mismatches = [2]

    str_examples = [
        ("AfterPreprocessing(<function parity>, Equals(1))",
         AfterPreprocessing(parity, Equals(1))),
        ]

    describe_examples = [
        ("1 != 0: after <function parity> on 2", 2,
         AfterPreprocessing(parity, Equals(1))),
        ("1 != 0", 2,
         AfterPreprocessing(parity, Equals(1), annotate=False)),
        ]


class TestMismatchDecorator(TestCase):

    run_tests_with = FullStackRunTest

    def test_forwards_description(self):
        x = Mismatch("description", {'foo': 'bar'})
        decorated = MismatchDecorator(x)
        self.assertEqual(x.describe(), decorated.describe())

    def test_forwards_details(self):
        x = Mismatch("description", {'foo': 'bar'})
        decorated = MismatchDecorator(x)
        self.assertEqual(x.get_details(), decorated.get_details())

    def test_repr(self):
        x = Mismatch("description", {'foo': 'bar'})
        decorated = MismatchDecorator(x)
        self.assertEqual(
            '<testtools.matchers.MismatchDecorator(%r)>' % (x,),
            repr(decorated))


class TestAllMatch(TestCase, TestMatchersInterface):

    matches_matcher = AllMatch(LessThan(10))
    matches_matches = [
        [9, 9, 9],
        (9, 9),
        iter([9, 9, 9, 9, 9]),
        ]
    matches_mismatches = [
        [11, 9, 9],
        iter([9, 12, 9, 11]),
        ]

    str_examples = [
        ("AllMatch(LessThan(12))", AllMatch(LessThan(12))),
        ]

    describe_examples = [
        ('Differences: [\n'
         '10 is not > 11\n'
         '10 is not > 10\n'
         ']',
         [11, 9, 10],
         AllMatch(LessThan(10))),
        ]


class PathHelpers(object):

    def mkdtemp(self):
        directory = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, directory)
        return directory

    def create_file(self, filename, contents=''):
        fp = open(filename, 'w')
        try:
            fp.write(contents)
        finally:
            fp.close()

    def touch(self, filename):
        return self.create_file(filename)


class TestPathExists(TestCase, PathHelpers):

    def test_exists(self):
        tempdir = self.mkdtemp()
        self.assertThat(tempdir, PathExists())

    def test_not_exists(self):
        doesntexist = os.path.join(self.mkdtemp(), 'doesntexist')
        mismatch = PathExists().match(doesntexist)
        self.assertThat(
            "%s does not exist." % doesntexist, Equals(mismatch.describe()))


class TestDirExists(TestCase, PathHelpers):

    def test_exists(self):
        tempdir = self.mkdtemp()
        self.assertThat(tempdir, DirExists())

    def test_not_exists(self):
        doesntexist = os.path.join(self.mkdtemp(), 'doesntexist')
        mismatch = DirExists().match(doesntexist)
        self.assertThat(
            PathExists().match(doesntexist).describe(),
            Equals(mismatch.describe()))

    def test_not_a_directory(self):
        filename = os.path.join(self.mkdtemp(), 'foo')
        self.touch(filename)
        mismatch = DirExists().match(filename)
        self.assertThat(
            "%s is not a directory." % filename, Equals(mismatch.describe()))


class TestFileExists(TestCase, PathHelpers):

    def test_exists(self):
        tempdir = self.mkdtemp()
        filename = os.path.join(tempdir, 'filename')
        self.touch(filename)
        self.assertThat(filename, FileExists())

    def test_not_exists(self):
        doesntexist = os.path.join(self.mkdtemp(), 'doesntexist')
        mismatch = FileExists().match(doesntexist)
        self.assertThat(
            PathExists().match(doesntexist).describe(),
            Equals(mismatch.describe()))

    def test_not_a_file(self):
        tempdir = self.mkdtemp()
        mismatch = FileExists().match(tempdir)
        self.assertThat(
            "%s is not a file." % tempdir, Equals(mismatch.describe()))


class TestDirContains(TestCase, PathHelpers):

    def test_empty(self):
        tempdir = self.mkdtemp()
        self.assertThat(tempdir, DirContains([]))

    def test_not_exists(self):
        doesntexist = os.path.join(self.mkdtemp(), 'doesntexist')
        mismatch = DirContains([]).match(doesntexist)
        self.assertThat(
            PathExists().match(doesntexist).describe(),
            Equals(mismatch.describe()))

    def test_contains_files(self):
        tempdir = self.mkdtemp()
        self.touch(os.path.join(tempdir, 'foo'))
        self.touch(os.path.join(tempdir, 'bar'))
        self.assertThat(tempdir, DirContains(['bar', 'foo']))

    def test_matcher(self):
        tempdir = self.mkdtemp()
        self.touch(os.path.join(tempdir, 'foo'))
        self.touch(os.path.join(tempdir, 'bar'))
        self.assertThat(tempdir, DirContains(matcher=Contains('bar')))

    def test_neither_specified(self):
        self.assertRaises(AssertionError, DirContains)

    def test_both_specified(self):
        self.assertRaises(
            AssertionError, DirContains, filenames=[], matcher=Contains('a'))

    def test_does_not_contain_files(self):
        tempdir = self.mkdtemp()
        self.touch(os.path.join(tempdir, 'foo'))
        mismatch = DirContains(['bar', 'foo']).match(tempdir)
        self.assertThat(
            Equals(['bar', 'foo']).match(['foo']).describe(),
            Equals(mismatch.describe()))


class TestFileContains(TestCase, PathHelpers):

    def test_not_exists(self):
        doesntexist = os.path.join(self.mkdtemp(), 'doesntexist')
        mismatch = FileContains('').match(doesntexist)
        self.assertThat(
            PathExists().match(doesntexist).describe(),
            Equals(mismatch.describe()))

    def test_contains(self):
        tempdir = self.mkdtemp()
        filename = os.path.join(tempdir, 'foo')
        self.create_file(filename, 'Hello World!')
        self.assertThat(filename, FileContains('Hello World!'))

    def test_matcher(self):
        tempdir = self.mkdtemp()
        filename = os.path.join(tempdir, 'foo')
        self.create_file(filename, 'Hello World!')
        self.assertThat(
            filename, FileContains(matcher=DocTestMatches('Hello World!')))

    def test_neither_specified(self):
        self.assertRaises(AssertionError, FileContains)

    def test_both_specified(self):
        self.assertRaises(
            AssertionError, FileContains, contents=[], matcher=Contains('a'))

    def test_does_not_contain(self):
        tempdir = self.mkdtemp()
        filename = os.path.join(tempdir, 'foo')
        self.create_file(filename, 'Goodbye Cruel World!')
        mismatch = FileContains('Hello World!').match(filename)
        self.assertThat(
            Equals('Hello World!').match('Goodbye Cruel World!').describe(),
            Equals(mismatch.describe()))


def is_even(x):
    return x % 2 == 0


class TestMatchesPredicate(TestCase, TestMatchersInterface):

    matches_matcher = MatchesPredicate(is_even, "%s is not even")
    matches_matches = [2, 4, 6, 8]
    matches_mismatches = [3, 5, 7, 9]

    str_examples = [
        ("MatchesPredicate(%r, %r)" % (is_even, "%s is not even"),
         MatchesPredicate(is_even, "%s is not even")),
        ]

    describe_examples = [
        ('7 is not even', 7, MatchesPredicate(is_even, "%s is not even")),
        ]


class TestTarballContains(TestCase, PathHelpers):

    def test_match(self):
        tempdir = self.mkdtemp()
        in_temp_dir = lambda x: os.path.join(tempdir, x)
        self.touch(in_temp_dir('a'))
        self.touch(in_temp_dir('b'))
        tarball = tarfile.open(in_temp_dir('foo.tar.gz'), 'w')
        tarball.add(in_temp_dir('a'), 'a')
        tarball.add(in_temp_dir('b'), 'b')
        tarball.close()
        self.assertThat(
            in_temp_dir('foo.tar.gz'), TarballContains(['b', 'a']))

    def test_mismatch(self):
        tempdir = self.mkdtemp()
        in_temp_dir = lambda x: os.path.join(tempdir, x)
        self.touch(in_temp_dir('a'))
        self.touch(in_temp_dir('b'))
        tarball = tarfile.open(in_temp_dir('foo.tar.gz'), 'w')
        tarball.add(in_temp_dir('a'), 'a')
        tarball.add(in_temp_dir('b'), 'b')
        tarball.close()
        mismatch = TarballContains(['d', 'c']).match(in_temp_dir('foo.tar.gz'))
        self.assertEqual(
            mismatch.describe(),
            Equals(['c', 'd']).match(['a', 'b']).describe())


class TestSamePath(TestCase, PathHelpers):

    def test_same_string(self):
        self.assertThat('foo', SamePath('foo'))

    def test_relative_and_absolute(self):
        path = 'foo'
        abspath = os.path.abspath(path)
        self.assertThat(path, SamePath(abspath))
        self.assertThat(abspath, SamePath(path))

    def test_real_path(self):
        symlink = getattr(os, 'symlink', None)
        skipIf(symlink is None, "No symlink support")
        tempdir = self.mkdtemp()
        source = os.path.join(tempdir, 'source')
        self.touch(source)
        target = os.path.join(tempdir, 'target')
        symlink(source, target)
        self.assertThat(source, SamePath(target))
        self.assertThat(target, SamePath(source))


class TestHasPermissions(TestCase, PathHelpers):

    def test_match(self):
        tempdir = self.mkdtemp()
        filename = os.path.join(tempdir, 'filename')
        self.touch(filename)
        permissions = oct(os.stat(filename).st_mode)[-4:]
        self.assertThat(filename, HasPermissions(permissions))


def test_suite():
    from unittest import TestLoader
    return TestLoader().loadTestsFromName(__name__)
