#include <QtTest/QtTest>

class SampleMathTests : public QObject
{
    Q_OBJECT

private slots:
    void testAddition()
    {
        QCOMPARE(1 + 1, 2);
        QCOMPARE(10 + (-3), 7);
    }

    void testSubtraction()
    {
        QCOMPARE(5 - 3, 2);
        QCOMPARE(0 - 5, -5);
    }

    void testMultiplication()
    {
        QCOMPARE(3 * 4, 12);
        QCOMPARE(-2 * 6, -12);
    }

    void testDivision()
    {
        QCOMPARE(10 / 2, 5);
        QVERIFY(10 % 3 == 1);
    }

    void testDivisionByZeroSafe()
    {
        // Demo of a deliberate failure so the GUI shows a red entry
        int divisor = 0;
        QVERIFY2(divisor != 0, "Divisor must not be zero");
    }

    void testBenchmark()
    {
        QBENCHMARK {
            volatile int sum = 0;
            for (int i = 0; i < 1000; ++i) sum += i;
            Q_UNUSED(sum)
        }
    }
};

QTEST_MAIN(SampleMathTests)
#include "SampleMathTests.moc"
