/*
    Copyright 2014-2015 Paul Colby

    This file is part of Bipolar.

    Bipolar is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Biplar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Bipolar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testtrainingsession.h"

#include "../../src/polar/v2/trainingsession.h"
#include "../../tools/variant.h"

#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTest>
#include <QXmlSchema>
#include <QXmlSchemaValidator>

Q_DECLARE_METATYPE(polar::v2::TrainingSession::OutputFormat)
Q_DECLARE_METATYPE(polar::v2::TrainingSession::OutputFormats)

// Qt's QDomDocument comparisons are based on references, and always fail when
// comparing two separate documents.  Additionally, the QDomDocument::toString
// and toByteArray functions are both non-deterministic. Hence we have to roll
// our own QDomDocument comparison functions :(

void compare(const QDomNode &a, const QDomNode &b);

void compare(const QDomNode &a, const QDomNamedNodeMap &b)
{
    if (a.namespaceURI().isEmpty()) {
        compare(a, b.namedItem(a.nodeName()));
    } else {
        compare(a, b.namedItemNS(a.namespaceURI(), a.localName()));
    }
}

void compare(const QDomNamedNodeMap &a, const QDomNamedNodeMap &b)
{
    QCOMPARE(a.length(), b.length());
    for (int i = 0; i < a.length(); ++i) {
        compare(a.item(i), b);
        compare(b.item(i), a);
    }
}

void compare(const QDomNodeList &a, const QDomNodeList &b)
{
    QCOMPARE(a.length(), b.length());
    for (int i = 0; i < a.length(); ++i) {
        compare(a.at(i), b.at(i));
    }
}

void fuzzyCompare(const QString &a, const QString &b, bool &compared)
{
    bool aOK, bOK;
    const double aDouble = a.toDouble(&aOK);
    const double bDouble = b.toDouble(&bOK);
    if (aOK && bOK) {
        compared = true;
        QCOMPARE(aDouble, bDouble);
    }
}

void compare(const QDomNode &a, const QDomNode &b)
{
    compare(a.attributes(), b.attributes());
    compare(a.childNodes(), b.childNodes());
    QCOMPARE(a.localName(), b.localName());
    QCOMPARE(a.namespaceURI(), b.namespaceURI());
    QCOMPARE(a.nodeName(), b.nodeName());
    QCOMPARE(a.nodeType(), b.nodeType());
    bool compared = false;
    fuzzyCompare(a.nodeValue(), b.nodeValue(), compared);
    if (!compared) {
        QCOMPARE(a.nodeValue(), b.nodeValue());
    }
    QCOMPARE(a.prefix(), b.prefix());
}

void compare(const QDomDocumentType &a, const QDomDocumentType &b)
{
    compare(static_cast<const QDomNode &>(a), static_cast<const QDomNode &>(b));
    compare(a.entities(), b.entities());
    QCOMPARE(a.internalSubset(), b.internalSubset());
    QCOMPARE(a.name(), b.name());
    QCOMPARE(a.nodeType(), b.nodeType());
    compare(a.notations(), b.notations());
    QCOMPARE(a.publicId(), b.publicId());
    QCOMPARE(a.systemId(), b.systemId());
}

void compare(const QDomDocument &a, const QDomDocument &b)
{
    compare(a.doctype(), b.doctype());
    compare(a.documentElement(), b.documentElement());
    QCOMPARE(a.nodeType(), b.nodeType());
}

void TestTrainingSession::getOutputBaseFileName_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("output");

    // Empty format strings use the training session base file name.
    QTest::newRow("empty-format-1")
        << QString::fromLatin1("abc")
        << QString()
        << QString::fromLatin1("abc");
    QTest::newRow("empty-format-2")
        << QString::fromLatin1("some/path/to/some/session/file")
        << QString()
        << QString::fromLatin1("file");
    QTest::newRow("empty-format-3")
        << QString::fromLatin1("/some/path/to/some/session/file")
        << QString()
        << QString::fromLatin1("file");

    // Check all of the fields that depend on a valid session base name.
    QTest::newRow("v2-users-12345678-training-sessions-23456789")
        << QString::fromLatin1("v2-users-12345678-training-sessions-23456789")
        << QString::fromLatin1("$userId|$sessionId")
        << QString::fromLatin1("12345678|23456789");

    // Check that multiple placeholder instances are replaced appropriately.
    QTest::newRow("v2-users-12345678-training-sessions-23456789")
        << QString::fromLatin1("v2-users-12345678-training-sessions-23456789")
        << QString::fromLatin1("$userId|$userId|$userId|$userId")
        << QString::fromLatin1("12345678|12345678|12345678|12345678");

    // Check all of the fields that depend on parsed session-create data.
    QTest::newRow("training-sessions-19946380-create")
        << QFINDTESTDATA("testdata/training-sessions-19946380-create")
        << QString::fromLatin1(
           "|1|$baseName"
           "|2|$date"
           "|3|$dateUTC"
           "|A|$dateExt"
           "|B|$dateExtUTC"
           "|4|$time"
           "|5|$timeUTC"
           "|C|$timeExt"
           "|D|$timeExtUTC"
           "|7|$sessionName"
           "|8|$invalid"
           "|9|$$$foo")
        << QString::fromLatin1(
           "|1|training-sessions-19946380"
           "|2|20140718"
           "|3|20140717"
           "|A|2014-07-18"
           "|B|2014-07-17"
           "|4|074856"
           "|5|214856"
           "|C|07:48:56"
           "|D|21:48:56"
           "|7|Other outdoor"
           "|8|$invalid"
           "|9|$$$foo");
    QTest::newRow("training-sessions-19946380-create")
        << QFINDTESTDATA("testdata/training-sessions-22165267-create")
        << QString::fromLatin1(
            "|1|$baseName"
            "|2|$date"
            "|3|$dateUTC"
            "|A|$dateExt"
            "|B|$dateExtUTC"
            "|4|$time"
            "|5|$timeUTC"
            "|C|$timeExt"
            "|D|$timeExtUTC"
            "|7|$sessionName"
            "|8|$invalid"
            "|9|$$$foo")
        << QString::fromLatin1(
            "|1|training-sessions-22165267"
            "|2|20140807"
            "|3|20140807"
            "|A|2014-08-07"
            "|B|2014-08-07"
            "|4|172501"
            "|5|072501"
            "|C|17:25:01"
            "|D|07:25:01"
            "|7|Running"
            "|8|$invalid"
            "|9|$$$foo");
}

void TestTrainingSession::getOutputBaseFileName()
{
    QFETCH(QString, input);
    QFETCH(QString, format);
    QFETCH(QString, output);
    if (input.endsWith(QLatin1String("-create"))) {
        input.chop(7);
    }
    polar::v2::TrainingSession session(input);
    QCOMPARE(session.getOutputBaseFileName(format), output);
}

void TestTrainingSession::getOutputFileNames_data()
{
    QTest::addColumn<QString>("inputBaseName");
    QTest::addColumn<QString>("outputFileNameFormat");
    QTest::addColumn<QString>("outputDirName");
    QTest::addColumn<polar::v2::TrainingSession::OutputFormats>("outputFileFormats");
    QTest::addColumn<QStringList>("outputFileNames");

    using namespace polar::v2;

    {   // GPX only.
        QStringList list;
        list.append(QLatin1String("test-dir/training-sessions-19946380.gpx"));
        QTest::newRow("gpx1")
            << QFINDTESTDATA("testdata/training-sessions-19946380-create")
            << QString()
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::GpxOutput)
            << list;
    } {
        QStringList list;
        list.append(QLatin1String("test-dir/training-sessions-22165267.gpx"));
        QTest::newRow("gpx2")
            << QFINDTESTDATA("testdata/training-sessions-22165267-create")
            << QString()
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::GpxOutput)
            << list;
    }

    {   // HRM only.
        QStringList list;
        list.append(QLatin1String("test-dir/training-sessions-19946380.hrm"));
        list.append(QLatin1String("test-dir/training-sessions-19946380.rr.hrm"));
        QTest::newRow("hrm1")
            << QFINDTESTDATA("testdata/training-sessions-19946380-create")
            << QString()
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::HrmOutput)
            << list;
    } {
        QStringList list;
        list.append(QLatin1String("test-dir/training-sessions-22165267.hrm"));
        list.append(QLatin1String("test-dir/training-sessions-22165267.rr.hrm"));
        QTest::newRow("hrm2")
            << QFINDTESTDATA("testdata/training-sessions-22165267-create")
            << QString()
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::HrmOutput)
            << list;
    }

    {   // TCX only.
        QStringList list;
        list.append(QLatin1String("test-dir/training-sessions-19946380.tcx"));
        QTest::newRow("tcx1")
            << QFINDTESTDATA("testdata/training-sessions-19946380-create")
            << QString()
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::TcxOutput)
            << list;
    } {
        QStringList list;
        list.append(QLatin1String("test-dir/training-sessions-22165267.tcx"));
        QTest::newRow("tcx2")
            << QFINDTESTDATA("testdata/training-sessions-22165267-create")
            << QString()
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::TcxOutput)
            << list;
    }

    {   // All output formats together.
        QStringList list;
        list.append(QLatin1String("test-dir/training-sessions-19946380.gpx"));
        list.append(QLatin1String("test-dir/training-sessions-19946380.hrm"));
        list.append(QLatin1String("test-dir/training-sessions-19946380.rr.hrm"));
        list.append(QLatin1String("test-dir/training-sessions-19946380.tcx"));
        QTest::newRow("all")
            << QFINDTESTDATA("testdata/training-sessions-19946380-create")
            << QString()
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::AllOutputs)
            << list;
    }

    {   // Non-default output base file name.
        QStringList list;
        list.append(QLatin1String("test-dir/20140718 074856 Other outdoor.gpx"));
        list.append(QLatin1String("test-dir/20140718 074856 Other outdoor.hrm"));
        list.append(QLatin1String("test-dir/20140718 074856 Other outdoor.rr.hrm"));
        list.append(QLatin1String("test-dir/20140718 074856 Other outdoor.tcx"));
        QTest::newRow("format")
            << QFINDTESTDATA("testdata/training-sessions-19946380-create")
            << QString::fromLatin1("$date $time $sessionName")
            << QString::fromLatin1("test-dir")
            << TrainingSession::OutputFormats(TrainingSession::AllOutputs)
            << list;
    }
}

void TestTrainingSession::getOutputFileNames()
{
    QFETCH(QString, inputBaseName);
    QFETCH(QString, outputFileNameFormat);
    QFETCH(QString, outputDirName);
    QFETCH(polar::v2::TrainingSession::OutputFormats, outputFileFormats);
    QFETCH(QStringList, outputFileNames);

    if (inputBaseName.endsWith(QLatin1String("-create"))) {
        inputBaseName.chop(7);
    }

    polar::v2::TrainingSession session(inputBaseName);
    session.setHrmOption(polar::v2::TrainingSession::RrFiles);
    QCOMPARE(session.getOutputFileNames(outputFileNameFormat, outputFileFormats,
             outputDirName), outputFileNames);
}

void TestTrainingSession::isGzipped_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<bool>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile dataFile(QFINDTESTDATA("testdata/" name ".gz")); \
        dataFile.open(QIODevice::ReadOnly); \
        QFile expectedFile(QFINDTESTDATA("testdata/" name)); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << dataFile.readAll() << true; \
    }

    LOAD_TEST_DATA("lorem-ipsum.txt");
    LOAD_TEST_DATA("random-bytes");

    #undef LOAD_TEST_DATA

    QTest::newRow("empty")   << QByteArray() << false;
    QTest::newRow("invalid") << QByteArray("\x11\x22\x33") << false;
}

void TestTrainingSession::isGzipped()
{
    QFETCH(QByteArray, data);
    QFETCH(bool, expected);

    QCOMPARE(polar::v2::TrainingSession::isGzipped(data), expected);
}

void TestTrainingSession::parseCreateExercise_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-create");
    LOAD_TEST_DATA("training-sessions-19946380-exercises-19896844-create");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-create");
    LOAD_TEST_DATA("training-sessions-42261903-exercises-42415131-create");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseCreateExercise()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseCreateExercise(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parseCreateSession_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-19946380-create");
    LOAD_TEST_DATA("training-sessions-22165267-create");
    LOAD_TEST_DATA("training-sessions-42261903-create");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseCreateSession()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseCreateSession(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parseLaps_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-autolaps");
    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-laps");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-autolaps");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-laps");
    LOAD_TEST_DATA("training-sessions-42261903-exercises-42415131-laps");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseLaps()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseLaps(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parsePhysicalInformation_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-19401412-physical-information");
    LOAD_TEST_DATA("training-sessions-19946380-physical-information");
    LOAD_TEST_DATA("training-sessions-22165267-physical-information");
    LOAD_TEST_DATA("training-sessions-42261903-physical-information");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parsePhysicalInformation()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parsePhysicalInformation(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parseRoute_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-1-exercises-1-route");
    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-route");
    LOAD_TEST_DATA("training-sessions-19946380-exercises-19896844-route");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-route");
    LOAD_TEST_DATA("training-sessions-42261903-exercises-42415131-route");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseRoute()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseRoute(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parseRRSamples_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-rrsamples");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-rrsamples");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseRRSamples()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseRRSamples(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
             .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parseSamples_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-1-exercises-1-samples");
    LOAD_TEST_DATA("training-sessions-2-exercises-1-samples");
    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-samples");
    LOAD_TEST_DATA("training-sessions-19946380-exercises-19896844-samples");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-samples");
    LOAD_TEST_DATA("training-sessions-42261903-exercises-42415131-samples");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseSamples()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseSamples(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
             .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parseStatistics_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-statistics");
    LOAD_TEST_DATA("training-sessions-19946380-exercises-19896844-statistics");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-statistics");
    LOAD_TEST_DATA("training-sessions-42261903-exercises-42415131-statistics");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseStatistics()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseStatistics(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
             .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::parseZones_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QVariantMap>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".expected.var")); \
        expectedFile.open(QIODevice::ReadOnly); \
        QDataStream expectedStream(&expectedFile); \
        QVariantMap expectedMap; \
        expectedStream >> expectedMap; \
        QTest::newRow(name) << QFINDTESTDATA("testdata/" name) << expectedMap; \
    }

    LOAD_TEST_DATA("training-sessions-2-exercises-1-zones");
    LOAD_TEST_DATA("training-sessions-19401412-exercises-19344289-zones");
    LOAD_TEST_DATA("training-sessions-19946380-exercises-19896844-zones");
    LOAD_TEST_DATA("training-sessions-22165267-exercises-22141894-zones");
    LOAD_TEST_DATA("training-sessions-42261903-exercises-42415131-zones");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::parseZones()
{
    QFETCH(QString, fileName);
    QFETCH(QVariantMap, expected);

    QVERIFY2(!fileName.isEmpty(), "failed to find testdata");

    // Parse the route (protobuf) message.
    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    const QVariantMap result = session.parseZones(fileName);

    // Write the result to files for optional post-mortem investigations.
    tools::variant::writeAll(result,
        QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QString::fromLatin1(QTest::currentDataTag())));

    // Compare the result.
    QCOMPARE(result, expected);
}

void TestTrainingSession::toGPX_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".gpx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(4); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toGPX()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    QDomDocument gpx = session.toGPX(QDateTime::fromString(
        QLatin1String("2014-07-15T12:34:56Z"), Qt::ISODate));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.gpx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.gpx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(gpx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(gpx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    gpx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    QFile xsd(QFINDTESTDATA("schemata/gpx.xsd"));
    QVERIFY(xsd.open(QIODevice::ReadOnly));
    QXmlSchema schema;
    QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
    QXmlSchemaValidator validator(schema);
    QVERIFY(validator.validate(gpx.toByteArray()));
}

void TestTrainingSession::toGPX_AllExtensions_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".all-extensions.gpx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(19); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toGPX_AllExtensions()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setGpxOption(polar::v2::TrainingSession::CluetrustGpxDataExtension);
    session.setGpxOption(polar::v2::TrainingSession::GarminAccelerationExtension);
    session.setGpxOption(polar::v2::TrainingSession::GarminTrackPointExtension);
    QDomDocument gpx = session.toGPX(QDateTime::fromString(
        QLatin1String("2014-07-15T12:34:56Z"), Qt::ISODate));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.all-extensions.gpx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.all-extensions.gpx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(gpx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(gpx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    gpx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    QFile xsd(QFINDTESTDATA("schemata/gpx.xsd"));
    QVERIFY(xsd.open(QIODevice::ReadOnly));
    QXmlSchema schema;
    QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
    QXmlSchemaValidator validator(schema);
    QVERIFY(validator.validate(gpx.toByteArray()));

    // The individual extension schemas will each be verified by their own
    // respective test functions, such as toGPX_Cluetrust.
}

void TestTrainingSession::toGPX_Cluetrust_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".cluetrust.gpx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(14); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toGPX_Cluetrust()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setGpxOption(polar::v2::TrainingSession::CluetrustGpxDataExtension);
    QDomDocument gpx = session.toGPX(QDateTime::fromString(
        QLatin1String("2014-07-15T12:34:56Z"), Qt::ISODate));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.cluetrust.gpx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.cluetrust.gpx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(gpx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(gpx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    gpx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    {   // The base GPX schema.
        QFile xsd(QFINDTESTDATA("schemata/gpx.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        QVERIFY(validator.validate(gpx.toByteArray()));
    }

    {   // The Cluetrust GPXData Extension.
        const QDomNodeList extensionNodes =
            gpx.elementsByTagName(QLatin1String("extensions"));
        QFile xsd(QFINDTESTDATA("schemata/gpxdata10.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        /// @todo  Set a custom network access manager to prevent
        ///        QXmlSchema::load fetching the GPX schema (gpx.xsd) remotely.
        //schema.setNetworkAccessManager(...);
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        for (int index = 0; index < extensionNodes.length(); ++index) {
            const QDomNodeList &extensions = extensionNodes.at(index).childNodes();
            for (int index = 0; index < extensions.length(); ++index) {
                QDomElement node = extensions.at(index).toElement();
                QVERIFY(node.nodeName().startsWith(QLatin1String("gpxdata")));
                node.setAttribute(QLatin1String("xmlns:gpxdata"),
                                  QLatin1String("http://www.cluetrust.com/XML/GPXDATA/1/0"));
                QByteArray byteArray;
                QTextStream stream(&byteArray);
                stream << node;
                QVERIFY(validator.validate(byteArray));
            }
        }
    }
}

void TestTrainingSession::toGPX_GarminAcceleration_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".garmin-acceleration.gpx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(24); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toGPX_GarminAcceleration()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setGpxOption(polar::v2::TrainingSession::GarminAccelerationExtension);
    QDomDocument gpx = session.toGPX(QDateTime::fromString(
        QLatin1String("2014-07-15T12:34:56Z"), Qt::ISODate));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.garmin-acceleration.gpx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.garmin-acceleration.gpx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(gpx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(gpx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    gpx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    {   // The base GPX schema.
        QFile xsd(QFINDTESTDATA("schemata/gpx.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        QVERIFY(validator.validate(gpx.toByteArray()));
    }

    {   // The Garmin Acceleration Extension.
        const QDomNodeList extensionNodes =
            gpx.elementsByTagName(QLatin1String("gpxax:AccelerationExtension"));
        QFile xsd(QFINDTESTDATA("schemata/AccelerationExtensionv1.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        for (int index = 0; index < extensionNodes.length(); ++index) {
            QDomElement node = extensionNodes.at(index).toElement();
            QVERIFY(node.nodeName().startsWith(QLatin1String("gpxax")));
            node.setAttribute(QLatin1String("xmlns:gpxax"),
                              QLatin1String("http://www.garmin.com/xmlschemas/AccelerationExtension/v1"));
            QByteArray byteArray;
            QTextStream stream(&byteArray);
            stream << node;
            QVERIFY(validator.validate(byteArray));
        }
    }
}

void TestTrainingSession::toGPX_GarminTrackPoint_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".garmin-trackpoint.gpx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(22); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toGPX_GarminTrackPoint()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setGpxOption(polar::v2::TrainingSession::GarminTrackPointExtension);
    QDomDocument gpx = session.toGPX(QDateTime::fromString(
        QLatin1String("2014-07-15T12:34:56Z"), Qt::ISODate));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.garmin-trackpoint.gpx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.garmin-trackpoint.gpx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(gpx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(gpx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    gpx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    {   // The base GPX schema.
        QFile xsd(QFINDTESTDATA("schemata/gpx.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        QVERIFY(validator.validate(gpx.toByteArray()));
    }

    {   // The Garmin TrackPoint Extension.
        const QDomNodeList extensionNodes =
            gpx.elementsByTagName(QLatin1String("gpxtpx:TrackPointExtension"));
        QFile xsd(QFINDTESTDATA("schemata/TrackPointExtensionv1.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        for (int index = 0; index < extensionNodes.length(); ++index) {
            QDomElement node = extensionNodes.at(index).toElement();
            QVERIFY(node.nodeName().startsWith(QLatin1String("gpxtpx")));
            node.setAttribute(QLatin1String("xmlns:gpxtpx"),
                              QLatin1String("http://www.garmin.com/xmlschemas/TrackPointExtension/v1"));
            QByteArray byteArray;
            QTextStream stream(&byteArray);
            stream << node;
            QVERIFY(validator.validate(byteArray));
        }
    }
}

void TestTrainingSession::toHRM_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QStringList>("expected");

    #define LOAD_TEST_DATA(name, expectedCount) { \
        QString baseName; \
        QStringList expected; \
        for (int count = 0; count < expectedCount; ++count) { \
            QFile expectedFile(QFINDTESTDATA((expectedCount == 1) \
                ? QString::fromLatin1("testdata/" name ".hrm") \
                : QString::fromLatin1("testdata/" name ".%1.hrm").arg(count) \
            )); \
            expectedFile.open(QIODevice::ReadOnly); \
            expected.append(QString::fromLatin1(expectedFile.readAll())); \
            if (baseName.isEmpty()) { \
                baseName = expectedFile.fileName(); \
                baseName.chop((expectedCount == 1) ? 4 : 6); \
            } \
        } \
        QTest::newRow(name) << baseName << expected; \
    }

    LOAD_TEST_DATA("training-sessions-19401412", 1);
    LOAD_TEST_DATA("training-sessions-19946380", 1);
    LOAD_TEST_DATA("training-sessions-22165267", 1);
    LOAD_TEST_DATA("training-sessions-42261903", 1);

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toHRM()
{
    QFETCH(QString, baseName);
    QFETCH(QStringList, expected);
    qDebug() << baseName;

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setHrmOption(polar::v2::TrainingSession::LapNames, false);
    const QStringList hrm = session.toHRM(false);

    // Write the result to a text file for optional post-mortem investigations.
    for (int index = 0; index < hrm.length(); ++index) {
        QString fileName;
#ifndef Q_OS_WIN
        fileName = QLatin1String("../");
#endif
        fileName += QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QLatin1String(QTest::currentDataTag()));
        if (hrm.length() != 1) {
            fileName += QString::fromLatin1(".%1").arg(index);
        }
        fileName += QString::fromLatin1(".hrm");
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
            file.write(hrm.at(index).toLatin1());
        }
    }

    // Compare the generated HRM string against the expected result.
    QCOMPARE(hrm, expected);
}

void TestTrainingSession::toHRM_LapNames_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QStringList>("expected");

    #define LOAD_TEST_DATA(name, expectedCount) { \
        QString baseName; \
        QStringList expected; \
        for (int count = 0; count < expectedCount; ++count) { \
            QFile expectedFile(QFINDTESTDATA((expectedCount == 1) \
                ? QString::fromLatin1("testdata/" name ".LapNames.hrm") \
                : QString::fromLatin1("testdata/" name ".%1.LapNames.hrm").arg(count) \
            )); \
            expectedFile.open(QIODevice::ReadOnly); \
            expected.append(QString::fromLatin1(expectedFile.readAll())); \
            if (baseName.isEmpty()) { \
                baseName = expectedFile.fileName(); \
                baseName.chop((expectedCount == 1) ? 13 : 15); \
            } \
        } \
        QTest::newRow(name) << baseName << expected; \
    }

    LOAD_TEST_DATA("training-sessions-19401412", 1);
    LOAD_TEST_DATA("training-sessions-19946380", 1);
    LOAD_TEST_DATA("training-sessions-22165267", 1);
    LOAD_TEST_DATA("training-sessions-42261903", 1);

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toHRM_LapNames()
{
    QFETCH(QString, baseName);
    QFETCH(QStringList, expected);
    qDebug() << baseName;

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setHrmOption(polar::v2::TrainingSession::LapNames);
    const QStringList hrm = session.toHRM(false);

    // Write the result to a text file for optional post-mortem investigations.
    for (int index = 0; index < hrm.length(); ++index) {
        QString fileName;
#ifndef Q_OS_WIN
        fileName = QLatin1String("../");
#endif
        fileName += QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QLatin1String(QTest::currentDataTag()));
        if (hrm.length() != 1) {
            fileName += QString::fromLatin1(".%1").arg(index);
        }
        fileName += QString::fromLatin1(".LapNames.hrm");
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
            file.write(hrm.at(index).toLatin1());
        }
    }

    // Compare the generated HRM string against the expected result.
    QCOMPARE(hrm, expected);
}

void TestTrainingSession::toHRM_LapNames_RR_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QStringList>("expected");

    #define LOAD_TEST_DATA(name, expectedCount) { \
        QString baseName; \
        QStringList expected; \
        for (int count = 0; count < expectedCount; ++count) { \
            QFile expectedFile(QFINDTESTDATA((expectedCount == 1) \
                ? QString::fromLatin1("testdata/" name ".rr.LapNames.hrm") \
                : QString::fromLatin1("testdata/" name ".%1.rr.LapNames.hrm").arg(count) \
            )); \
            expectedFile.open(QIODevice::ReadOnly); \
            expected.append(QString::fromLatin1(expectedFile.readAll())); \
            if (baseName.isEmpty()) { \
                baseName = expectedFile.fileName(); \
                baseName.chop((expectedCount == 1) ? 16 : 18); \
            } \
        } \
        QTest::newRow(name) << baseName << expected; \
    }

    LOAD_TEST_DATA("training-sessions-19401412", 1);
    LOAD_TEST_DATA("training-sessions-19946380", 1);
    LOAD_TEST_DATA("training-sessions-22165267", 1);
    LOAD_TEST_DATA("training-sessions-42261903", 1);

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toHRM_LapNames_RR()
{
    QFETCH(QString, baseName);
    QFETCH(QStringList, expected);
    qDebug() << baseName;

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setHrmOption(polar::v2::TrainingSession::LapNames);
    const QStringList hrm = session.toHRM(true);

    // Write the result to a text file for optional post-mortem investigations.
    for (int index = 0; index < hrm.length(); ++index) {
        QString fileName;
#ifndef Q_OS_WIN
        fileName = QLatin1String("../");
#endif
        fileName += QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QLatin1String(QTest::currentDataTag()));
        if (hrm.length() != 1) {
            fileName += QString::fromLatin1(".%1").arg(index);
        }
        fileName += QString::fromLatin1(".rr.LapNames.hrm");
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
            file.write(hrm.at(index).toLatin1());
        }
    }

    // Compare the generated HRM string against the expected result.
    QCOMPARE(hrm, expected);
}

void TestTrainingSession::toHRM_RR_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QStringList>("expected");

    #define LOAD_TEST_DATA(name, expectedCount) { \
        QString baseName; \
        QStringList expected; \
        for (int count = 0; count < expectedCount; ++count) { \
            QFile expectedFile(QFINDTESTDATA((expectedCount == 1) \
                ? QString::fromLatin1("testdata/" name ".rr.hrm") \
                : QString::fromLatin1("testdata/" name ".%1.rr.hrm").arg(count) \
            )); \
            expectedFile.open(QIODevice::ReadOnly); \
            expected.append(QString::fromLatin1(expectedFile.readAll())); \
            if (baseName.isEmpty()) { \
                baseName = expectedFile.fileName(); \
                baseName.chop((expectedCount == 1) ? 7 : 9); \
            } \
        } \
        QTest::newRow(name) << baseName << expected; \
    }

    LOAD_TEST_DATA("training-sessions-19401412", 1);
    LOAD_TEST_DATA("training-sessions-19946380", 1);
    LOAD_TEST_DATA("training-sessions-22165267", 1);
    LOAD_TEST_DATA("training-sessions-42261903", 1);

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toHRM_RR()
{
    QFETCH(QString, baseName);
    QFETCH(QStringList, expected);
    qDebug() << baseName;

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setHrmOption(polar::v2::TrainingSession::LapNames, false);
    const QStringList hrm = session.toHRM(true);

    // Write the result to a text file for optional post-mortem investigations.
    for (int index = 0; index < hrm.length(); ++index) {
        QString fileName;
#ifndef Q_OS_WIN
        fileName = QLatin1String("../");
#endif
        fileName += QString::fromLatin1("polar/v2/testdata/%1.result")
            .arg(QLatin1String(QTest::currentDataTag()));
        if (hrm.length() != 1) {
            fileName += QString::fromLatin1(".%1").arg(index);
        }
        fileName += QString::fromLatin1(".rr.hrm");
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
            file.write(hrm.at(index).toLatin1());
        }
    }

    // Compare the generated HRM string against the expected result.
    QCOMPARE(hrm, expected);
}

void TestTrainingSession::toTCX_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".tcx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(4); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toTCX()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    QDomDocument tcx = session.toTCX(QLatin1String("Jul 17 2014 21:02:38"));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.tcx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.tcx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(tcx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(tcx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    tcx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    QFile xsd(QFINDTESTDATA("schemata/TrainingCenterDatabasev2.xsd"));
    QVERIFY(xsd.open(QIODevice::ReadOnly));
    QXmlSchema schema;
    QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
    QXmlSchemaValidator validator(schema);
    QVERIFY(validator.validate(tcx.toByteArray()));
}

void TestTrainingSession::toTCX_AllExtensions_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".all-extensions.tcx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(19); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toTCX_AllExtensions()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setTcxOption(polar::v2::TrainingSession::GarminActivityExtension);
    session.setTcxOption(polar::v2::TrainingSession::GarminCourseExtension);
    QDomDocument tcx = session.toTCX(QLatin1String("Jul 17 2014 21:02:38"));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.all-extensions.tcx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.all-extensions.tcx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(tcx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(tcx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    tcx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    QFile xsd(QFINDTESTDATA("schemata/TrainingCenterDatabasev2.xsd"));
    QVERIFY(xsd.open(QIODevice::ReadOnly));
    QXmlSchema schema;
    QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
    QXmlSchemaValidator validator(schema);
    QVERIFY(validator.validate(tcx.toByteArray()));

    // The individual extension schemas will each be verified by their own
    // respective test functions, such as toTCX_GarminActivity.
}

void TestTrainingSession::toTCX_GarminActivity_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".garmin-activity.tcx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(20); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toTCX_GarminActivity()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setTcxOption(polar::v2::TrainingSession::GarminActivityExtension);
    QDomDocument tcx = session.toTCX(QLatin1String("Jul 17 2014 21:02:38"));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.garmin-activity.tcx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.garmin-activity.tcx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(tcx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(tcx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    tcx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    {   // The base TCX V2 schema.
        QFile xsd(QFINDTESTDATA("schemata/TrainingCenterDatabasev2.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        QVERIFY(validator.validate(tcx.toByteArray()));
    }

    {   // The Garmin Activity Extension V2 schema's TPX elements.
        const QDomNodeList tpxNodes = tcx.elementsByTagName(QLatin1String("TPX"));
        QFile xsd(QFINDTESTDATA("schemata/ActivityExtensionv2.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        for (int index = 0; index < tpxNodes.length(); ++index) {
            QByteArray byteArray;
            QTextStream stream(&byteArray);
            stream << tpxNodes.at(index);
            QVERIFY(validator.validate(byteArray));
        }
    }

    {   // The Garmin Activity Extension V2 schema's LX elements.
        const QDomNodeList lxNodes = tcx.elementsByTagName(QLatin1String("LX"));
        QFile xsd(QFINDTESTDATA("schemata/ActivityExtensionv2.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        for (int index = 0; index < lxNodes.length(); ++index) {
            QByteArray byteArray;
            QTextStream stream(&byteArray);
            stream << lxNodes.at(index);
            QVERIFY(validator.validate(byteArray));
        }
    }
}

void TestTrainingSession::toTCX_GarminCourse_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".garmin-course.tcx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(18); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toTCX_GarminCourse()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setTcxOption(polar::v2::TrainingSession::GarminCourseExtension);
    QDomDocument tcx = session.toTCX(QLatin1String("Jul 17 2014 21:02:38"));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.garmin-course.tcx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.garmin-course.tcx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(tcx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(tcx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    tcx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    {   // The base TCX V2 schema.
        QFile xsd(QFINDTESTDATA("schemata/TrainingCenterDatabasev2.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        QVERIFY(validator.validate(tcx.toByteArray()));
    }

    {   // The Garmin Course Extension V1 schema's CX elements.
        const QDomNodeList cxNodes = tcx.elementsByTagName(QLatin1String("CX"));
        QFile xsd(QFINDTESTDATA("schemata/CourseExtensionv1.xsd"));
        QVERIFY(xsd.open(QIODevice::ReadOnly));
        QXmlSchema schema;
        QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
        QXmlSchemaValidator validator(schema);
        for (int index = 0; index < cxNodes.length(); ++index) {
            QByteArray byteArray;
            QTextStream stream(&byteArray);
            stream << cxNodes.at(index);
            QVERIFY(validator.validate(byteArray));
        }
    }
}

void TestTrainingSession::toTCX_UTC_data()
{
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile expectedFile(QFINDTESTDATA("testdata/" name ".utc.tcx")); \
        QString baseName(expectedFile.fileName()); \
        baseName.chop(8); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << baseName << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("training-sessions-1");
    LOAD_TEST_DATA("training-sessions-2");
    LOAD_TEST_DATA("training-sessions-19401412");
    LOAD_TEST_DATA("training-sessions-19946380");
    LOAD_TEST_DATA("training-sessions-22165267");
    LOAD_TEST_DATA("training-sessions-42261903");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::toTCX_UTC()
{
    QFETCH(QString, baseName);
    QFETCH(QByteArray, expected);

    // Parse the route (protobuf) message.
    polar::v2::TrainingSession session(baseName);
    QVERIFY(session.parse());
    session.setTcxOption(polar::v2::TrainingSession::ForceTcxUTC);
    QDomDocument tcx = session.toTCX(QLatin1String("Jul 17 2014 21:02:38"));

    // Write the result to an XML file for optional post-mortem investigations.
#ifdef Q_OS_WIN
    QFile file(QString::fromLatin1("polar/v2/testdata/%1.result.utc.tcx")
#else
    QFile file(QString::fromLatin1("../polar/v2/testdata/%1.result.utc.tcx")
#endif
        .arg(QString::fromLatin1(QTest::currentDataTag())));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        file.write(tcx.toByteArray(2));
    }
    file.close();

    // Compare the generated document against the expected result.
    QDomDocument expectedDoc;
    expectedDoc.setContent(expected);
    compare(tcx, expectedDoc);

    // Validate the generated document against the relevant XML schema.
    tcx.documentElement().removeAttribute(QLatin1String("xsi:schemaLocation"));
    QFile xsd(QFINDTESTDATA("schemata/TrainingCenterDatabasev2.xsd"));
    QVERIFY(xsd.open(QIODevice::ReadOnly));
    QXmlSchema schema;
    QVERIFY(schema.load(&xsd, QUrl::fromLocalFile(xsd.fileName())));
    QXmlSchemaValidator validator(schema);
    QVERIFY(validator.validate(tcx.toByteArray()));
}

void TestTrainingSession::unzip_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("expected");

    #define LOAD_TEST_DATA(name) { \
        QFile dataFile(QFINDTESTDATA("testdata/" name ".gz")); \
        dataFile.open(QIODevice::ReadOnly); \
        QFile expectedFile(QFINDTESTDATA("testdata/" name)); \
        expectedFile.open(QIODevice::ReadOnly); \
        QTest::newRow(name) << dataFile.readAll() << expectedFile.readAll(); \
    }

    LOAD_TEST_DATA("lorem-ipsum.txt");
    LOAD_TEST_DATA("random-bytes");

    #undef LOAD_TEST_DATA
}

void TestTrainingSession::unzip()
{
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, expected);

    QVERIFY2(!data.isEmpty(), "failed to load testdata");
    QVERIFY2(!expected.isEmpty(), "failed to load testdata");

    const polar::v2::TrainingSession session(QLatin1String("ignored"));
    QCOMPARE(session.unzip(data), expected);   // Default initial buffer size.
    QCOMPARE(session.unzip(data,1), expected); // Tiny initial buffer size.
}
