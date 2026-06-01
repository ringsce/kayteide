#include <QtTest/QtTest>
#include <QString>

class SampleStringTests : public QObject
{
    Q_OBJECT

private slots:
    void testToUpper()
    {
        QCOMPARE(QString("hello").toUpper(), QString("HELLO"));
    }

    void testToLower()
    {
        QCOMPARE(QString("WORLD").toLower(), QString("world"));
    }

    void testContains()
    {
        QVERIFY(QString("Qt6 is great").contains("Qt6"));
        QVERIFY(!QString("Qt6 is great").contains("Qt5"));
    }

    void testSplit()
    {
        QStringList parts = QString("a,b,c").split(',');
        QCOMPARE(parts.size(), 3);
        QCOMPARE(parts.at(0), QString("a"));
        QCOMPARE(parts.at(2), QString("c"));
    }

    void testReplace()
    {
        QString s = "Hello World";
        s.replace("World", "Qt6");
        QCOMPARE(s, QString("Hello Qt6"));
    }

    void testTrimmed()
    {
        QCOMPARE(QString("  spaces  ").trimmed(), QString("spaces"));
    }

    void testStartsEndsWith()
    {
        QString s = "TestRunner";
        QVERIFY(s.startsWith("Test"));
        QVERIFY(s.endsWith("Runner"));
    }

    // Intentionally failing test to demo the GUI failure highlight
    void testIntentionalFailure()
    {
        QCOMPARE(QString("foo"), QString("bar"));
    }
};

QTEST_MAIN(SampleStringTests)
#include "SampleStringTests.moc"
