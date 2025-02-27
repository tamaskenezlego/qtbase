// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/private/qcomparisontesthelper_p.h>

#include <QtXml/qdom.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qlist.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qvariant.h>
#include <QtCore/qxmlstream.h>

#include <QtXml/private/qdom_p.h>

#include <cmath>

QT_REQUIRE_CONFIG(dom);
QT_FORWARD_DECLARE_CLASS(QDomDocument)
QT_FORWARD_DECLARE_CLASS(QDomNode)

using namespace Qt::StringLiterals;

class tst_QDom : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void namespacedAttributes() const;
    void setContent_data();
    void setContent();
    void setContentOverloads();
    void parseOptions();
    void spacingOnlyNodes_data() const;
    void spacingOnlyNodes() const;
    void parseResult();
    void toString_01_data();
    void toString_01();
    void toString_02_data();
    void toString_02();
    void hasAttributes_data();
    void hasAttributes();
    void setGetAttributes();
    void save_data();
    void save();
    void saveWithSerialization() const;
    void saveWithSerialization_data() const;
    void cloneNode_data();
    void cloneNode();
    void ownerDocument_data();
    void ownerDocument();
    void ownerDocumentTask27424_data();
    void ownerDocumentTask27424();
    void parentNode_data();
    void parentNode();
    void documentCreationTask27424_data();
    void documentCreationTask27424();
    void browseElements();
    void ownerElementTask45192_data();
    void ownerElementTask45192();
    void domNodeMapAndList();

    void nullDocument();
    void invalidName_data();
    void invalidName();
    void invalidQualifiedName_data();
    void invalidQualifiedName();
    void invalidCharData_data();
    void invalidCharData();
    void nonBMPCharacters();

    void roundTripAttributes() const;
    void roundTripCDATA() const;
    void normalizeEndOfLine() const;
    void normalizeAttributes() const;
    void serializeWeirdEOL() const;
    void reparentAttribute() const;
    void serializeNamespaces() const;
    void flagInvalidNamespaces() const;
    void flagUndeclaredNamespace() const;

    void indentComments() const;
    void checkLiveness() const;
    void reportDuplicateAttributes() const;
    void appendChildFromToDocument() const;
    void iterateCDATA() const;
    void appendDocumentNode() const;
    void germanUmlautToByteArray() const;
    void germanUmlautToFile() const;
    void setInvalidDataPolicy() const;
    void crashInSetContent() const;
    void doubleNamespaceDeclarations() const;
    void setContentQXmlReaderOverload() const;
    void toStringWithoutNewlines() const;
    void checkIntOverflow() const;
    void setContentWhitespace() const;
    void setContentWhitespace_data() const;
    void setContentUnopenedQIODevice() const;

    void taskQTBUG4595_dontAssertWhenDocumentSpecifiesUnknownEncoding() const;
    void cloneDTD_QTBUG8398() const;
    void DTDNotationDecl();
    void DTDEntityDecl();
    void DTDInternalSubset() const;
    void DTDInternalSubset_data() const;
    void QTBUG49113_dontCrashWithNegativeIndex() const;
    void standalone();
    void splitTextLeakMemory() const;

    void testDomListComparisonCompiles();
    void testDomListComparison_data();
    void testDomListComparison();

    void cleanupTestCase() const;

private:
    static QDomDocument generateRequest();
    static int hasAttributesHelper( const QDomNode& node );
    static bool compareDocuments( const QDomDocument &doc1, const QDomDocument &doc2 );
    static bool compareNodes( const QDomNode &node1, const QDomNode &node2, bool deep );
    static QDomNode findDomNode( const QDomDocument &doc, const QList<QVariant> &pathToNode );
    static QString onNullWarning(const char *const functionName);
    static bool isDeepEqual(const QDomNode &n1, const QDomNode &n2);
    static bool isFakeXMLDeclaration(const QDomNode &node);

    QList<QByteArray> m_testCodecs;
};


void tst_QDom::setContent_data()
{
    const QString doc01(
        "<!DOCTYPE a1 [ <!ENTITY blubber 'and'> ]>\n"
        "<a1>\n"
        " <b1>\n"
        "  <c1>foo</c1>\n"
        "  <c2>bar</c2>\n"
        "  <c3>foo &amp; bar</c3>\n"
        "  <c4>foo &blubber; bar</c4>\n"
        " </b1>\n"
        " <b2> </b2>\n"
        " <b3>\n"
        "  <c1/>\n"
        " </b3>\n"
        "</a1>\n"
        );

    QTest::addColumn<QString>("doc");
    QTest::addColumn<QStringList>("featuresTrue");
    QTest::addColumn<QStringList>("featuresFalse");
    QTest::addColumn<QString>("res");

    QTest::newRow( "01" ) << doc01
                       << QStringList()
                       << QString("http://trolltech.com/xml/features/report-whitespace-only-CharData").split(' ')
                       << QString("<!DOCTYPE a1>\n"
                                   "<a1>\n"
                                   "    <b1>\n"
                                   "        <c1>foo</c1>\n"
                                   "        <c2>bar</c2>\n"
                                   "        <c3>foo &amp; bar</c3>\n"
                                   "        <c4>foo and bar</c4>\n"
                                   "    </b1>\n"
                                   "    <b2/>\n"
                                   "    <b3>\n"
                                   "        <c1/>\n"
                                   "    </b3>\n"
                                   "</a1>\n");

    QTest::newRow("02") << QString("<message>\n"
                                "    <body>&lt;b&gt;foo&lt;/b&gt;>]]&gt;</body>\n"
                                "</message>\n")
                     << QStringList() << QStringList()
                     << QString("<message>\n"
                                "    <body>&lt;b>foo&lt;/b>>]]&gt;</body>\n"
                                "</message>\n");

}

void tst_QDom::setContent()
{
    QFETCH( QString, doc );

    QDomDocument domDoc;
    QXmlStreamReader reader(doc);
#if QT_DEPRECATED_SINCE(6, 8)
    QT_IGNORE_DEPRECATIONS(domDoc.setContent(&reader, true);)
#else
    QVERIFY(domDoc.setContent(&reader, QDomDocument::ParseOption::UseNamespaceProcessing));
#endif // QT_DEPRECATED_SINCE(6, 8)

    QString eRes;
    QTextStream ts( &eRes, QIODevice::WriteOnly );
    domDoc.save( ts, 4 );

    QTEST( eRes, "res" );

    // make sure that if we parse our output again, we get the same document
    QDomDocument domDoc1;
    QDomDocument domDoc2;
    QVERIFY( domDoc1.setContent( doc ) );
    QVERIFY( domDoc2.setContent( eRes ) );
    QVERIFY( compareDocuments( domDoc1, domDoc2 ) );

    // Test overload taking a QAnyStringView
    QDomDocument domFromStringView;
    QStringView stringView(doc);
    QVERIFY(domFromStringView.setContent(stringView));
    QVERIFY(compareDocuments(domFromStringView, domDoc));

    // Test overload taking a QByteArray
    QDomDocument domFromByteArary;
    QByteArray byteArray = doc.toUtf8();
    QVERIFY(domFromByteArary.setContent(byteArray));
    QVERIFY(compareDocuments(domFromByteArary, domDoc));

    // Test overload taking a QIODevice
    QDomDocument domFromIODevice;
    QBuffer buffer(&byteArray);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QVERIFY(domFromIODevice.setContent(&buffer));
    QVERIFY(compareDocuments(domFromIODevice, domDoc));
}

void tst_QDom::setContentOverloads()
{
    QDomDocument doc;
    QString errorMessage;

    QByteArray data("<a>test</a>");
    QString text = QString::fromLatin1(data);
    QXmlStreamReader reader(data);

    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QVERIFY(doc.setContent(data));
    QVERIFY(doc.setContent(data.data()));
    QVERIFY(doc.setContent(text));
    QVERIFY(doc.setContent(&reader));
    QVERIFY(doc.setContent(&buffer));
    buffer.reset();

    // With parse options
    QVERIFY(doc.setContent(data, QDomDocument::ParseOption::Default));
    QVERIFY(doc.setContent(data.data(), QDomDocument::ParseOption::Default));
    QVERIFY(doc.setContent(text, QDomDocument::ParseOption::Default));
    QVERIFY(doc.setContent(&reader, QDomDocument::ParseOption::Default));
    QVERIFY(doc.setContent(&buffer, QDomDocument::ParseOption::Default));
    buffer.reset();

#if QT_DEPRECATED_SINCE(6, 8)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    // With output param
    QVERIFY(doc.setContent(data, &errorMessage));
    QVERIFY(doc.setContent(text, &errorMessage));
    QVERIFY(doc.setContent(&buffer, &errorMessage));
    buffer.reset();
    // There's no setContent(QXmlStreamReader *, QString *, int *, int *) overload
    // QVERIFY(doc.setContent(&reader, &errorMessage));

    // With namespace processing param
    QVERIFY(doc.setContent(data, false));
    QVERIFY(doc.setContent(text, false));
    QVERIFY(doc.setContent(&reader, false));
    QVERIFY(doc.setContent(&buffer, false));
    buffer.reset();

    // With namespace processing and output params
    QVERIFY(doc.setContent(data, false, &errorMessage));
    QVERIFY(doc.setContent(text, false, &errorMessage));
    QVERIFY(doc.setContent(&reader, false, &errorMessage));
    QVERIFY(doc.setContent(&buffer, false, &errorMessage));
    buffer.reset();
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 8)
}

void tst_QDom::parseOptions()
{
    QString input = u"<doc xmlns:ns='http://example.com/'>"
                    "<ns:nested/>"
                    "</doc>"_s;
    {
        QDomDocument document;
        QVERIFY(document.setContent(input, QDomDocument::ParseOption::Default));
        const QDomElement docElement = document.firstChildElement("doc");
        QVERIFY(!docElement.isNull());
        const QDomElement nestedElement = docElement.firstChildElement();
        QCOMPARE(nestedElement.nodeName(), "ns:nested");
        QVERIFY(!nestedElement.isNull());
        QVERIFY(nestedElement.localName().isEmpty());
        QVERIFY(nestedElement.prefix().isEmpty());
        QVERIFY(nestedElement.namespaceURI().isEmpty());
    }
    {
        QDomDocument document;
        QVERIFY(document.setContent(input, QDomDocument::ParseOption::UseNamespaceProcessing));
        const QDomElement docElement = document.firstChildElement("doc");
        QVERIFY(!docElement.isNull());
        const QDomElement nestedElement = docElement.firstChildElement("nested");
        QVERIFY(!nestedElement.isNull());
        QCOMPARE(nestedElement.nodeName(), "ns:nested");
        QCOMPARE(nestedElement.localName(), "nested");
        QCOMPARE(nestedElement.prefix(), "ns");
        QCOMPARE(nestedElement.namespaceURI(), "http://example.com/");
    }
}

void tst_QDom::spacingOnlyNodes_data() const
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QDomDocument::ParseOption>("options");

    QTest::newRow("spacing-only-remove")
            << u"<a> \t \n \r</a>"_s
            << u"<a/>"_s
            << QDomDocument::ParseOption::Default;
    // \r is translated to \n, see https://www.w3.org/TR/xml11/#sec-line-ends
    QTest::newRow("spacing-only-preserve")
            << u"<a> \t \n \r</a>"_s
            << u"<a> \t \n \n</a>"_s
            << QDomDocument::ParseOption::PreserveSpacingOnlyNodes;
    QTest::newRow("mixed-text-remove")
            << u"<a> abc \t \n \r</a>"_s
            << u"<a> abc \t \n \n</a>"_s
            << QDomDocument::ParseOption::Default;
    QTest::newRow("mixed-text-preserve")
            << u"<a> abc \t \n \r</a>"_s
            << u"<a> abc \t \n \n</a>"_s
            << QDomDocument::ParseOption::PreserveSpacingOnlyNodes;

    // QDomDocument treats all chacarcters below as spaces (see QTBUG-105348)
    static constexpr char16_t spaces[] = {
        QChar::Space, QChar::Tabulation, QChar::LineFeed,
        QChar::CarriageReturn, QChar::Nbsp,
        0x2002, // EN SPACE
        0x2003, // EM SPACE
        0x2009  // THIN SPACE
    };

    for (char16_t space : spaces) {
        QTest::addRow("spacing-remove-u%04x", space)
                << u"<a>"_s + space + u"</a>"_s
                << u"<a/>"_s
                << QDomDocument::ParseOption::Default;

        // \r is translated to \n, see https://www.w3.org/TR/xml11/#sec-line-ends
        char16_t expected = (space == QChar::CarriageReturn) ? char16_t(QChar::LineFeed) : space;
        QTest::addRow("spacing-preserve-u%04x", space)
                << u"<a>"_s + space + u"</a>"_s
                << u"<a>"_s + expected + u"</a>"_s
                << QDomDocument::ParseOption::PreserveSpacingOnlyNodes;
    }
}

void tst_QDom::spacingOnlyNodes() const
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QFETCH(QDomDocument::ParseOption, options);

    QDomDocument doc;
    QVERIFY(doc.setContent(input, options));
    QCOMPARE(doc.toString(-1), expected);
}

void tst_QDom::parseResult()
{
    QString input = u"<doc xmlns:b='http://example.com/'>"
                    "<b:element/>"_s;

    QDomDocument document;
    QDomDocument::ParseResult result = document.setContent(input);
    QVERIFY(!result);
    QVERIFY(!result.errorMessage.isEmpty());
    QVERIFY(result.errorLine);
    QVERIFY(result.errorColumn);

    input.append("</doc>");
    result = document.setContent(input);
    QVERIFY(result);
    QVERIFY(result.errorMessage.isEmpty());
    QVERIFY(!result.errorLine);
    QVERIFY(!result.errorColumn);
}

void tst_QDom::toString_01_data()
{
    QTest::addColumn<QString>("fileName");
    const QString prefix = QFINDTESTDATA("testdata/toString_01");
    if (prefix.isEmpty())
        QFAIL("Cannot find testdata directory!");
    QTest::newRow( "01" ) << QString(prefix + "/doc01.xml");
    QTest::newRow( "02" ) << QString(prefix + "/doc02.xml");
    QTest::newRow( "03" ) << QString(prefix + "/doc03.xml");
    QTest::newRow( "04" ) << QString(prefix + "/doc04.xml");
    QTest::newRow( "05" ) << QString(prefix + "/doc05.xml");

    QTest::newRow( "little-endian" ) << QString(prefix + "/doc_little-endian.xml");
    QTest::newRow( "utf-16" ) << QString(prefix + "/doc_utf-16.xml");
    QTest::newRow( "utf-8" ) << QString(prefix + "/doc_utf-8.xml");

}

/*! \internal

  This function tests that the QDomDocument::toString() function results in the
  same XML document. The meaning of "same" in this context means that the
  "information" in the resulting XML file is the same as in the original, i.e.
  we are not intrested in different formatting, etc.

  To achieve this, the XML document of the toString() function is parsed again
  and the two QDomDocuments are compared.
*/
void tst_QDom::toString_01()
{
    QFETCH(QString, fileName);

    QFile f(fileName);
    QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(QString::fromLatin1("Failed to open file %1, file error: %2").arg(fileName).arg(f.error())));

    QDomDocument doc;
    QDomDocument::ParseResult result = doc.setContent(&f, QDomDocument::ParseOption::Default);
    QVERIFY2(result,
             qPrintable(u"QDomDocument::setContent() failed (%1:%2): %3"_s.arg(result.errorLine)
                                .arg(result.errorColumn)
                                .arg(result.errorMessage)));
    // test toString()'s invariant with different indenting depths
    for ( int i=0; i<5; i++ ) {
        QString toStr = doc.toString( i );

        QDomDocument res;
        QVERIFY( res.setContent( toStr ) );

        QVERIFY( compareDocuments( doc, res ) );
    }
}

void tst_QDom::toString_02_data()
{
    save_data();
}

/*
  Tests the new QDomDocument::toString(int) overload (basically the same test
  as save()).
*/
void tst_QDom::toString_02()
{
    QFETCH( QString, doc );
    QFETCH( int, indent );

    QDomDocument domDoc;
    QVERIFY( domDoc.setContent( doc ) );
    QTEST( domDoc.toString(indent), "res" );
}


void tst_QDom::hasAttributes_data()
{
    QTest::addColumn<int>("visitedNodes");
    QTest::addColumn<QByteArray>("xmlDoc");

    QByteArray doc1("<top>Make a <blubb>stupid</blubb>, useless test sentence.</top>");
    QByteArray doc2("<top a=\"a\">Make a <blubb a=\"a\">stupid</blubb>, useless test sentence.</top>");
    QByteArray doc3("<!-- just a useless comment -->\n"
                    "<?pi foo bar?>\n"
                    "<foo>\n"
                    "<bar fnord=\"snafu\" hmpf=\"grmpf\">\n"
                    "<foobar/>\n"
                    "</bar>\n"
                    "<bar>blubber</bar>\n"
                    "more text, pretty unintresting, though\n"
                    "<hmpfl blubber=\"something\" />\n"
                    "<![CDATA[ foo bar @!<>] ]]>\n"
                    "</foo>\n"
                    "<!-- just a useless comment -->\n"
                    "<?pi foo bar?>\n");

    QTest::newRow( "01" ) << 6 << doc1;
    QTest::newRow( "02" ) << 6 << doc2;
    QTest::newRow( "03" ) << 13 << doc3;
}

/*
  This function tests that QDomNode::hasAttributes() returns true if and only
  if the node has attributes (i.e. QDomNode::attributes() returns a list with
  attributes in it).
*/
void tst_QDom::hasAttributes()
{
    QFETCH( QByteArray, xmlDoc );

    QDomDocument doc;
    QVERIFY( doc.setContent( xmlDoc ) );

    int visitedNodes = hasAttributesHelper( doc );
    QTEST( visitedNodes, "visitedNodes" );
}

void tst_QDom::setGetAttributes()
{
    QDomDocument doc;
    QDomElement rootNode = doc.createElement("Root");
    doc.appendChild(rootNode);

    const auto restoreDefault = qScopeGuard([prior = QLocale()]() {
        QLocale::setDefault(prior);
    });
    QLocale::setDefault(QLocale::German); // decimal separator != '.'

    const QString qstringVal("QString");
    const qlonglong qlonglongVal = std::numeric_limits<qlonglong>::min();
    const qulonglong qulonglongVal = std::numeric_limits<qulonglong>::max();
    const int intVal = std::numeric_limits<int>::min();
    const uint uintVal = std::numeric_limits<uint>::max();
    const float floatVal = 0.1234f;
    const double doubleVal1 = 1./6.;
    const double doubleVal2 = std::nextafter(doubleVal1, 1.);
    const double doubleVal3 = std::nextafter(doubleVal2, 1.);

    rootNode.setAttribute("qstringVal", qstringVal);
    rootNode.setAttribute("qlonglongVal", qlonglongVal);
    rootNode.setAttribute("qulonglongVal", qulonglongVal);
    rootNode.setAttribute("intVal", intVal);
    rootNode.setAttribute("uintVal", uintVal);
    rootNode.setAttribute("floatVal", floatVal);
    rootNode.setAttribute("doubleVal1", doubleVal1);
    rootNode.setAttribute("doubleVal2", doubleVal2);
    rootNode.setAttribute("doubleVal3", doubleVal3);

    QDomElement nsNode = doc.createElement("NS");
    rootNode.appendChild(nsNode);
    nsNode.setAttributeNS("namespace", "qstringVal", qstringVal);
    nsNode.setAttributeNS("namespace", "qlonglongVal", qlonglongVal);
    nsNode.setAttributeNS("namespace", "qulonglongVal", qulonglongVal);
    nsNode.setAttributeNS("namespace", "intVal", intVal);
    nsNode.setAttributeNS("namespace", "uintVal", uintVal);
    nsNode.setAttributeNS("namespace", "floatVal", floatVal); // not available atm
    nsNode.setAttributeNS("namespace", "doubleVal1", doubleVal1);
    nsNode.setAttributeNS("namespace", "doubleVal2", doubleVal2);
    nsNode.setAttributeNS("namespace", "doubleVal3", doubleVal3);

    bool bOk;
    QCOMPARE(rootNode.attribute("qstringVal"), qstringVal);
    QCOMPARE(rootNode.attribute("qlonglongVal").toLongLong(&bOk), qlonglongVal);
    QVERIFY(bOk);
    QCOMPARE(rootNode.attribute("qulonglongVal").toULongLong(&bOk), qulonglongVal);
    QVERIFY(bOk);
    QCOMPARE(rootNode.attribute("intVal").toInt(&bOk), intVal);
    QVERIFY(bOk);
    QCOMPARE(rootNode.attribute("uintVal").toUInt(&bOk), uintVal);
    QVERIFY(bOk);
    QCOMPARE(rootNode.attribute("floatVal").toFloat(&bOk), floatVal);
    QVERIFY(bOk);

    QVERIFY(rootNode.attribute("doubleVal1").toDouble(&bOk) == doubleVal1 && bOk);
    QVERIFY(rootNode.attribute("doubleVal2").toDouble(&bOk) == doubleVal2 && bOk);
    QVERIFY(rootNode.attribute("doubleVal3").toDouble(&bOk) == doubleVal3 && bOk);

    QCOMPARE(nsNode.attributeNS("namespace", "qstringVal"), qstringVal);
    QCOMPARE(nsNode.attributeNS("namespace", "qlonglongVal").toLongLong(&bOk), qlonglongVal);
    QVERIFY(bOk);
    QCOMPARE(nsNode.attributeNS("namespace", "qulonglongVal").toULongLong(&bOk), qulonglongVal);
    QVERIFY(bOk);
    QCOMPARE(nsNode.attributeNS("namespace", "intVal").toInt(&bOk), intVal);
    QVERIFY(bOk);
    QCOMPARE(nsNode.attributeNS("namespace", "uintVal").toUInt(&bOk), uintVal);
    QVERIFY(bOk);
    QCOMPARE(nsNode.attributeNS("namespace", "floatVal").toFloat(&bOk), floatVal);
    QVERIFY(bOk);
    QVERIFY(nsNode.attributeNS("namespace", "doubleVal1").toDouble(&bOk) == doubleVal1 && bOk);
    QVERIFY(nsNode.attributeNS("namespace", "doubleVal2").toDouble(&bOk) == doubleVal2 && bOk);
    QVERIFY(nsNode.attributeNS("namespace", "doubleVal3").toDouble(&bOk) == doubleVal3 && bOk);
}


int tst_QDom::hasAttributesHelper( const QDomNode& node )
{
    int visitedNodes = 1;
    if ( node.hasAttributes() ) {
            if (node.attributes().count() == 0)
            return -1;
//        QVERIFY( node.attributes().count() > 0 );
    } else {
            if (node.attributes().count() != 0)
            return -1;
//        QVERIFY( node.attributes().count() == 0 );
    }

    QDomNodeList children = node.childNodes();
    for ( int i=0; i<children.count(); i++ ) {
            int j = hasAttributesHelper( children.item(i) );
        if (j < 0)
            return -1;
        visitedNodes += j;
    }
    return visitedNodes;
}


void tst_QDom::save_data()
{
    const QString doc01(
            "<a1>\n"
            " <b1>\n"
            "  <c1>\n"
            "   <d1/>\n"
            "  </c1>\n"
            "  <c2/>\n"
            " </b1>\n"
            " <b2/>\n"
            " <b3>\n"
            "  <c1/>\n"
            " </b3>\n"
            "</a1>\n"
            );

    QTest::addColumn<QString>("doc");
    QTest::addColumn<int>("indent");
    QTest::addColumn<QString>("res");

    QTest::newRow( "01" ) << doc01 << 0 << QString(doc01).replace( QRegularExpression(" "), "" );
    QTest::newRow( "02" ) << doc01 << 1 << doc01;
    QTest::newRow( "03" ) << doc01 << 2 << QString(doc01).replace( QRegularExpression(" "), "  " );
    QTest::newRow( "04" ) << doc01 << 10 << QString(doc01).replace( QRegularExpression(" "), "          " );
}

void tst_QDom::save()
{
    QFETCH( QString, doc );
    QFETCH( int, indent );

    QDomDocument domDoc;
    QVERIFY( domDoc.setContent( doc ) );

    QString eRes;
    QTextStream ts( &eRes, QIODevice::WriteOnly );
    domDoc.save( ts, indent );

    QTEST( eRes, "res" );
}

void tst_QDom::initTestCase()
{
    QString testFile = QFINDTESTDATA("testdata/testCodecs.txt");
    if (testFile.isEmpty())
        QFAIL("Cannot find testdata/testCodecs.txt");
    QFile file(testFile);
    QVERIFY(file.open(QIODevice::ReadOnly|QIODevice::Text));

    QByteArray codecName;

    m_testCodecs = file.readAll().split('\n');
    if (m_testCodecs.last().isEmpty())
        m_testCodecs.removeLast();

}

void tst_QDom::saveWithSerialization() const
{
    QFETCH(QString, fileName);

    QFile f(fileName);
    QVERIFY(f.open(QIODevice::ReadOnly));

    QDomDocument doc;

    // Read the document
    QVERIFY(doc.setContent(&f));


    QByteArray storage;
    QBuffer writeDevice(&storage);
    QVERIFY(writeDevice.open(QIODevice::WriteOnly));

    QTextStream s(&writeDevice);

    doc.save(s, 0, QDomNode::EncodingFromTextStream);
    s.flush();
    writeDevice.close();

    QBuffer readDevice(&storage);
    QVERIFY(readDevice.open(QIODevice::ReadOnly));

    QDomDocument result;
    QDomDocument::ParseResult parseResult =
            result.setContent(&readDevice, QDomDocument::ParseOption::Default);

    QVERIFY2(parseResult,
             qPrintable(QString::fromLatin1("Failed: line %2, column %3: %4, content: %5")
                                .arg(QString::number(parseResult.errorLine),
                                     QString::number(parseResult.errorColumn),
                                     parseResult.errorMessage, QString::fromUtf8(storage))));
    if (!compareDocuments(doc, result))
    {
        QCOMPARE(doc.toString(), result.toString());

        /* We put this one here as well, in case the QCOMPARE above for some strange reason
         * nevertheless succeeds. */
        QVERIFY2(false, qPrintable(QString::fromLatin1("Failed to serialize test data")));
    }
}

void tst_QDom::saveWithSerialization_data() const
{
    QTest::addColumn<QString>("fileName");
    const QString prefix = QFINDTESTDATA("testdata/toString_01");
    if (prefix.isEmpty())
        QFAIL("Cannot find testdata!");
    QTest::newRow("doc01.xml") << QString(prefix + "/doc01.xml");
    QTest::newRow("doc02.xml") << QString(prefix + "/doc02.xml");
    QTest::newRow("doc03.xml") << QString(prefix + "/doc03.xml");
    QTest::newRow("doc04.xml") << QString(prefix + "/doc04.xml");
    QTest::newRow("doc05.xml") << QString(prefix + "/doc05.xml");

    QTest::newRow("doc_little-endian.xml") << QString(prefix + "/doc_little-endian.xml");
    QTest::newRow("doc_utf-16.xml") << QString(prefix + "/doc_utf-16.xml");
    QTest::newRow("doc_utf-8.xml") << QString(prefix + "/doc_utf-8.xml");
}

void tst_QDom::cloneNode_data()
{
    const QString doc01(
            "<a1>\n"
            " <b1>\n"
            "  <c1>\n"
            "   <d1/>\n"
            "  </c1>\n"
            "  <c2/>\n"
            " </b1>\n"
            " <b2/>\n"
            " <b3>\n"
            "  <c1/>\n"
            " </b3>\n"
            "</a1>\n"
            );
    QList<QVariant> nodeB1;
    nodeB1 << 0;

    QList<QVariant> nodeC1;
    nodeC1 << 0 << 0;

    QList<QVariant> nodeC2;
    nodeC2 << 0 << 1;

    QTest::addColumn<QString>("doc");
    QTest::addColumn<QList<QVariant> >("pathToNode");
    QTest::addColumn<bool>("deep");

    QTest::newRow( "noDeep_01" ) << doc01 << nodeB1 << false;
    QTest::newRow( "noDeep_02" ) << doc01 << nodeC1 << false;
    QTest::newRow( "noDeep_03" ) << doc01 << nodeC2 << false;

    QTest::newRow( "deep_01" ) << doc01 << nodeB1 << true;
    QTest::newRow( "deep_02" ) << doc01 << nodeC1 << true;
    QTest::newRow( "deep_03" ) << doc01 << nodeC2 << true;
}

void tst_QDom::cloneNode()
{
    QFETCH( QString, doc );
    QFETCH( QList<QVariant>, pathToNode );
    QFETCH( bool, deep );
    QDomDocument domDoc;
    QVERIFY( domDoc.setContent( doc ) );
    QDomNode node = findDomNode( domDoc, pathToNode );
    QVERIFY(!node.isNull());

    QDomNode clonedNode = node.cloneNode( deep );
    QVERIFY( compareNodes( node, clonedNode, deep ) );

    QDomNode parent = node.parentNode();
    if ( !parent.isNull() ) {
        node = parent.replaceChild( clonedNode, node ); // swap the nodes
        QVERIFY( !node.isNull() );
        QVERIFY( compareNodes( node, clonedNode, deep ) );
    }
}


void tst_QDom::ownerElementTask45192_data()
{
    const QString doc(
        "<root>\n"
        " <item name=\"test\" >\n"
        " </item>\n"
        "</root>"
    );

    QTest::addColumn<QString>("doc");
    QTest::newRow("doc") << doc;
}

void tst_QDom::ownerElementTask45192()
{
    QFETCH( QString, doc );
    QDomDocument domDoc;
    QVERIFY( domDoc.setContent( doc ) );

    QDomNode item = domDoc.documentElement().firstChild();
    QDomNode clone = item.cloneNode(false);

    QVERIFY( clone == clone.attributes().namedItem("name").toAttr().ownerElement() );
}

void tst_QDom::ownerDocument_data()
{
    cloneNode_data();
}

#define OWNERDOCUMENT_CREATE_TEST( t, x ) \
{ \
    t n = x; \
    QVERIFY( n.ownerDocument() == domDoc ); \
}

#define OWNERDOCUMENT_IMPORTNODE_TEST( t, x ) \
{ \
    QDomNode importedNode; \
    t n = x; \
    QVERIFY( n.ownerDocument() != domDoc ); \
    importedNode = domDoc.importNode( n, deep ); \
    QVERIFY( n.ownerDocument() != domDoc ); \
    QVERIFY( importedNode.ownerDocument() == domDoc ); \
}

void tst_QDom::ownerDocument()
{
    QFETCH( QString, doc );
    QFETCH( QList<QVariant>, pathToNode );
    QFETCH( bool, deep );
    QDomDocument domDoc;
    QVERIFY( domDoc.setContent( doc ) );
    QDomNode node = findDomNode( domDoc, pathToNode );
    QVERIFY(!node.isNull());

    QVERIFY( node.ownerDocument() == domDoc );

    // Does cloneNode() keep the ownerDocument()?
    {
        QDomNode clonedNode = node.cloneNode( deep );
        QVERIFY( node.ownerDocument() == domDoc );
        QVERIFY( clonedNode.ownerDocument() == domDoc );
    }

    // If the original DOM node is replaced with the cloned node, does this
    // keep the ownerDocument()?
    {
        QDomNode clonedNode = node.cloneNode( deep );
        QDomNode parent = node.parentNode();
        if ( !parent.isNull() ) {
            node = parent.replaceChild( clonedNode, node ); // swap the nodes
            QVERIFY( node.ownerDocument() == domDoc );
            QVERIFY( clonedNode.ownerDocument() == domDoc );
        }
    }

    // test QDomDocument::create...()
    {
        OWNERDOCUMENT_CREATE_TEST( QDomAttr,                    domDoc.createAttribute( "foo" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomAttr,                    domDoc.createAttributeNS( "foo", "bar" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomCDATASection,            domDoc.createCDATASection( "foo" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomComment,                 domDoc.createComment( "foo" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomDocumentFragment,        domDoc.createDocumentFragment() );
        OWNERDOCUMENT_CREATE_TEST( QDomElement,                 domDoc.createElement( "foo" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomElement,                 domDoc.createElementNS( "foo", "bar" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomEntityReference,         domDoc.createEntityReference( "foo" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomProcessingInstruction,   domDoc.createProcessingInstruction( "foo", "bar" ) );
        OWNERDOCUMENT_CREATE_TEST( QDomText,                    domDoc.createTextNode( "foo" ) );
    }

    // test importNode()
    {
        QDomDocument doc2;
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomAttr,                    doc2.createAttribute( "foo" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomAttr,                    doc2.createAttributeNS( "foo", "bar" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomCDATASection,            doc2.createCDATASection( "foo" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomComment,                 doc2.createComment( "foo" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomDocumentFragment,        doc2.createDocumentFragment() );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomElement,                 doc2.createElement( "foo" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomElement,                 doc2.createElementNS( "foo", "bar" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomEntityReference,         doc2.createEntityReference( "foo" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomProcessingInstruction,   doc2.createProcessingInstruction( "foo", "bar" ) );
        OWNERDOCUMENT_IMPORTNODE_TEST( QDomText,                    doc2.createTextNode( "foo" ) );

        // QTBUG-12927
        QVERIFY(doc2.importNode(QDomNode(), deep).isNull());
    }
}

void tst_QDom::ownerDocumentTask27424_data()
{
    QTest::addColumn<bool>("insertLevel1AfterCstr");
    QTest::addColumn<bool>("insertLevel2AfterCstr");
    QTest::addColumn<bool>("insertLevel3AfterCstr");

    QTest::newRow( "000" ) << false << false << false;
    QTest::newRow( "001" ) << false << false << true;
    QTest::newRow( "010" ) << false << true  << false;
    QTest::newRow( "011" ) << false << true  << true;
    QTest::newRow( "100" ) << true  << false << false;
    QTest::newRow( "101" ) << true  << false << true;
    QTest::newRow( "110" ) << true  << true  << false;
    QTest::newRow( "111" ) << true  << true  << true;
}

void tst_QDom::ownerDocumentTask27424()
{
    QFETCH( bool, insertLevel1AfterCstr );
    QFETCH( bool, insertLevel2AfterCstr );
    QFETCH( bool, insertLevel3AfterCstr );

    QDomDocument doc("TestXML");

    QDomElement level1 = doc.createElement("Level_1");
    QVERIFY( level1.ownerDocument() == doc );

    if ( insertLevel1AfterCstr ) {
        doc.appendChild(level1);
        QVERIFY( level1.ownerDocument() == doc );
    }

    QDomElement level2 = level1.ownerDocument().createElement("Level_2");
    QVERIFY( level1.ownerDocument() == doc );
    QVERIFY( level2.ownerDocument() == doc );

    if ( insertLevel2AfterCstr ) {
        level1.appendChild(level2);
        QVERIFY( level1.ownerDocument() == doc );
        QVERIFY( level2.ownerDocument() == doc );
    }

    QDomElement level3 = level2.ownerDocument().createElement("Level_3");
    QVERIFY( level1.ownerDocument() == doc );
    QVERIFY( level2.ownerDocument() == doc );
    QVERIFY( level3.ownerDocument() == doc );

    if ( insertLevel3AfterCstr ) {
        level2.appendChild(level3);
        QVERIFY( level1.ownerDocument() == doc );
        QVERIFY( level2.ownerDocument() == doc );
        QVERIFY( level3.ownerDocument() == doc );
    }

    QDomNode level4 = level3.ownerDocument().createTextNode("This_is_a_value!");
    QVERIFY( level4.ownerDocument() == doc );

    level3.appendChild(level4);
    QVERIFY( level1.ownerDocument() == doc );
    QVERIFY( level2.ownerDocument() == doc );
    QVERIFY( level3.ownerDocument() == doc );
    QVERIFY( level4.ownerDocument() == doc );

    if ( !insertLevel3AfterCstr ) {
        level2.appendChild(level3);
        QVERIFY( level1.ownerDocument() == doc );
        QVERIFY( level2.ownerDocument() == doc );
        QVERIFY( level3.ownerDocument() == doc );
        QVERIFY( level4.ownerDocument() == doc );
    }

    if ( !insertLevel2AfterCstr ) {
        level1.appendChild(level2);
        QVERIFY( level1.ownerDocument() == doc );
        QVERIFY( level2.ownerDocument() == doc );
        QVERIFY( level3.ownerDocument() == doc );
        QVERIFY( level4.ownerDocument() == doc );
    }

    if ( !insertLevel1AfterCstr ) {
        doc.appendChild(level1);
        QVERIFY( level1.ownerDocument() == doc );
        QVERIFY( level2.ownerDocument() == doc );
        QVERIFY( level3.ownerDocument() == doc );
        QVERIFY( level4.ownerDocument() == doc );
    }
}

void tst_QDom::parentNode_data()
{
    cloneNode_data();
}

#define PARENTNODE_CREATE_TEST( t, x ) \
{ \
    t n = x; \
    QVERIFY( n.parentNode().isNull() ); \
}

void tst_QDom::parentNode()
{
    QFETCH( QString, doc );
    QFETCH( QList<QVariant>, pathToNode );
    QFETCH( bool, deep );
    QDomDocument domDoc;
    QVERIFY( domDoc.setContent( doc ) );
    QDomNode node = findDomNode( domDoc, pathToNode );
    QVERIFY(!node.isNull());
    Q_UNUSED(deep);

    // test QDomDocument::create...()
    {
        PARENTNODE_CREATE_TEST( QDomAttr,                   domDoc.createAttribute( "foo" ) );
        PARENTNODE_CREATE_TEST( QDomAttr,                   domDoc.createAttributeNS( "foo", "bar" ) );
        PARENTNODE_CREATE_TEST( QDomCDATASection,           domDoc.createCDATASection( "foo" ) );
        PARENTNODE_CREATE_TEST( QDomComment,                domDoc.createComment( "foo" ) );
        PARENTNODE_CREATE_TEST( QDomDocumentFragment,       domDoc.createDocumentFragment() );
        PARENTNODE_CREATE_TEST( QDomElement,                domDoc.createElement( "foo" ) );
        PARENTNODE_CREATE_TEST( QDomElement,                domDoc.createElementNS( "foo", "bar" ) );
        PARENTNODE_CREATE_TEST( QDomEntityReference,        domDoc.createEntityReference( "foo" ) );
        PARENTNODE_CREATE_TEST( QDomProcessingInstruction,  domDoc.createProcessingInstruction( "foo", "bar" ) );
        PARENTNODE_CREATE_TEST( QDomText,                   domDoc.createTextNode( "foo" ) );
    }
}


void tst_QDom::documentCreationTask27424_data()
{
    QTest::addColumn<bool>("insertLevel1AfterCstr");
    QTest::addColumn<bool>("insertLevel2AfterCstr");
    QTest::addColumn<bool>("insertLevel3AfterCstr");

    QTest::newRow( "000" ) << false << false << false;
    QTest::newRow( "001" ) << false << false << true;
    QTest::newRow( "010" ) << false << true  << false;
    QTest::newRow( "011" ) << false << true  << true;
    QTest::newRow( "100" ) << true  << false << false;
    QTest::newRow( "101" ) << true  << false << true;
    QTest::newRow( "110" ) << true  << true  << false;
    QTest::newRow( "111" ) << true  << true  << true;
}

void tst_QDom::documentCreationTask27424()
{
    QFETCH( bool, insertLevel1AfterCstr );
    QFETCH( bool, insertLevel2AfterCstr );
    QFETCH( bool, insertLevel3AfterCstr );

    QDomDocument docRes;
    QVERIFY( docRes.setContent( QString(
                "<!DOCTYPE TestXML>\n"
                "<Level_1>\n"
                " <Level_2>\n"
                "  <Level_3>This_is_a_value!</Level_3>\n"
                " </Level_2>\n"
                "</Level_1>"
                ) ) );

    QDomDocument doc("TestXML");

    QDomElement level1 = doc.createElement("Level_1");
    if ( insertLevel1AfterCstr )
        doc.appendChild(level1);

    QDomElement level2 = level1.ownerDocument().createElement("Level_2");
    if ( insertLevel2AfterCstr )
        level1.appendChild(level2);

    QDomElement level3 = level2.ownerDocument().createElement("Level_3");
    if ( insertLevel3AfterCstr )
        level2.appendChild(level3);

    QDomNode level4 = level3.ownerDocument().createTextNode("This_is_a_value!");
    level3.appendChild(level4);

    if ( !insertLevel3AfterCstr )
        level2.appendChild(level3);
    if ( !insertLevel2AfterCstr )
        level1.appendChild(level2);
    if ( !insertLevel1AfterCstr )
        doc.appendChild(level1);

    QVERIFY( compareDocuments( doc, docRes ) );
}


bool tst_QDom::isFakeXMLDeclaration(const QDomNode &node)
{
    return node.isProcessingInstruction() &&
           node.nodeName() == QLatin1String("xml");
}

bool tst_QDom::isDeepEqual(const QDomNode &n1, const QDomNode &n2)
{
    const QDomNode::NodeType nt = n1.nodeType();

    if(nt != n2.nodeType())
        return false;

    if(n1.nodeName() != n2.nodeName()
       || n1.namespaceURI() != n2.namespaceURI()
       || n1.nodeValue() != n2.nodeValue())
        return false;

    /* Check the children. */
    const QDomNodeList children1(n1.childNodes());
    const QDomNodeList children2(n2.childNodes());
    uint len1 = children1.length();
    uint len2 = children2.length();
    uint i1 = 0;
    uint i2 = 0;

    if(len1 != 0 && isFakeXMLDeclaration(children1.at(0)))
            ++i1;

    if(len2 != 0 && isFakeXMLDeclaration(children2.at(0)))
            ++i2;

    if(len1 - i1 != len2 - i2)
        return false;

    // We jump over the first to skip the processing instructions that
    // are (incorrectly) used as XML declarations.
    for(; i1 < len1; ++i1)
    {
        if(!isDeepEqual(children1.at(i1), children2.at(i2)))
            return false;

        ++i2;
    }

    return true;
}

/*
    Returns true if \a doc1 and \a doc2 represent the same XML document, i.e.
    they have the same informational content. Otherwise, this function returns
    false.
*/
bool tst_QDom::compareDocuments( const QDomDocument &doc1, const QDomDocument &doc2 )
{
    return isDeepEqual(doc1, doc2);
}

/*
    Returns true if \a node1 and \a node2 represent the same XML node, i.e.
    they have the same informational content. Otherwise, this function returns
    false.

    If \a deep is true, children of the nodes are also tested. If \a deep is
    false, only \a node1 and \a node 2 are compared.
*/
bool tst_QDom::compareNodes( const QDomNode &node1, const QDomNode &node2, bool deep )
{
    if ( deep ) {
        QString str1;
        {
            QTextStream stream( &str1 );
            stream << node1;
        }
        QString str2;
        {
            QTextStream stream( &str2 );
            stream << node2;
        }
        return str1 == str2;
    }

    if ( node1.isNull() && node2.isNull() )
        return true;
    // ### I am not sure if this test is complete
    bool equal =     node1.nodeName() == node2.nodeName();
    equal = equal && node1.nodeType() == node2.nodeType();
    equal = equal && node1.localName() == node2.localName();
    equal = equal && node1.nodeValue() == node2.nodeValue();
    equal = equal && node1.prefix() == node2.prefix();

    return equal;
}

/*
    \a pathToNode is a list of indices to wanted node in \a doc. Returns the
    wanted node.
*/
QDomNode tst_QDom::findDomNode( const QDomDocument &doc, const QList<QVariant> &pathToNode )
{
    QDomNode node = doc;
    QList<QVariant>::const_iterator it;
    for ( it = pathToNode.begin(); it != pathToNode.end(); ++it ) {
        QDomNodeList children = node.childNodes();
        node = children.item( (*it).toInt() );
//        QVERIFY( !node.isNull() );
    }
    return node;
}

void tst_QDom::browseElements()
{
    QDomDocument doc;
    QDomElement root = doc.createElement("foo");
    doc.appendChild(root);
    root.appendChild(doc.createElement("bar"));
    root.appendChild(doc.createElement("bop"));
    root.appendChild(doc.createElement("bar"));
    root.appendChild(doc.createElement("bop"));
    root.appendChild(doc.createElementNS("org.example.bar", "bar"));
    root.appendChild(doc.createElementNS("org.example.foo", "flup"));
    root.appendChild(doc.createElementNS("org.example.foo2", "flup"));
    root.appendChild(doc.createElementNS("org.example.bar", "bar2"));

    QVERIFY(doc.firstChildElement("ding").isNull());
    QDomElement foo = doc.firstChildElement("foo");
    QVERIFY(!foo.isNull());
    QVERIFY(foo.firstChildElement("ding").isNull());
    QVERIFY(foo.nextSiblingElement("foo").isNull());
    QVERIFY(foo.previousSiblingElement("bar").isNull());
    QVERIFY(foo.nextSiblingElement().isNull());
    QVERIFY(foo.previousSiblingElement().isNull());

    QDomElement bar = foo.firstChildElement("bar");
    QVERIFY(!bar.isNull());
    QCOMPARE(bar.namespaceURI(), QString());
    QVERIFY(bar.previousSiblingElement("bar").isNull());
    QVERIFY(bar.previousSiblingElement().isNull());
    QCOMPARE(bar.nextSiblingElement("bar").tagName(), QLatin1String("bar"));
    QCOMPARE(bar.nextSiblingElement("bar").nextSiblingElement("bar").tagName(), QLatin1String("bar"));
    QVERIFY(bar.nextSiblingElement("bar").nextSiblingElement("bar").nextSiblingElement("bar").isNull());

    QDomElement bop = foo.firstChildElement("bop");
    QVERIFY(!bop.isNull());
    QCOMPARE(bar.nextSiblingElement(), bop);
    QCOMPARE(bop.nextSiblingElement("bop"), foo.lastChildElement("bop"));
    QCOMPARE(bop.previousSiblingElement("bar"), foo.firstChildElement("bar"));
    QCOMPARE(bop.previousSiblingElement("bar"), foo.firstChildElement());

    bar = foo.firstChildElement("bar", "org.example.bar");
    QVERIFY(!bar.isNull());
    QCOMPARE(bar.tagName(), QLatin1String("bar"));
    QCOMPARE(bar.namespaceURI(), QLatin1String("org.example.bar"));
    QVERIFY(bar.nextSiblingElement("bar", "org.example.bar").isNull());
    QVERIFY(bar.nextSiblingElement("bar").isNull());
    QVERIFY(bar.previousSiblingElement("bar", "org.example.bar").isNull());
    QVERIFY(!bar.previousSiblingElement("bar").isNull());

    bar = foo.firstChildElement("bar", "");
    QCOMPARE(bar.namespaceURI(), QString());
    bar = foo.lastChildElement("bar");
    QCOMPARE(bar.namespaceURI(), QLatin1String("org.example.bar"));
    bar = foo.lastChildElement("bar", "");
    QCOMPARE(bar.namespaceURI(), QLatin1String("org.example.bar"));

    QVERIFY(foo.firstChildElement("bar", "abc").isNull());
    QVERIFY(foo.lastChildElement("bar", "abc").isNull());

    QDomElement barNS = foo.firstChildElement(QString(), "org.example.bar");
    QVERIFY(!barNS.isNull());
    QCOMPARE(barNS.tagName(), "bar");
    QVERIFY(!barNS.nextSiblingElement(QString(), "org.example.bar").isNull());
    QVERIFY(barNS.previousSiblingElement(QString(), "org.example.bar").isNull());

    barNS = foo.firstChildElement("", "org.example.bar");
    QVERIFY(!barNS.isNull());
    QCOMPARE(barNS.tagName(), "bar");
    QVERIFY(!barNS.nextSiblingElement("", "org.example.bar").isNull());
    QVERIFY(barNS.previousSiblingElement("", "org.example.bar").isNull());

    barNS = foo.lastChildElement(QString(), "org.example.bar");
    QVERIFY(!barNS.isNull());
    QCOMPARE(barNS.tagName(), "bar2");
    QVERIFY(barNS.nextSiblingElement(QString(), "org.example.bar").isNull());
    QVERIFY(!barNS.previousSiblingElement(QString(), "org.example.bar").isNull());

    barNS = foo.lastChildElement("", "org.example.bar");
    QVERIFY(!barNS.isNull());
    QCOMPARE(barNS.tagName(), "bar2");
    QVERIFY(barNS.nextSiblingElement("", "org.example.bar").isNull());
    QVERIFY(!barNS.previousSiblingElement("", "org.example.bar").isNull());

    QDomElement flup = foo.firstChildElement("flup");
    QVERIFY(!flup.isNull());
    QCOMPARE(flup.namespaceURI(), QLatin1String("org.example.foo"));
    QVERIFY(flup.previousSiblingElement("flup").isNull());
    QVERIFY(!flup.nextSiblingElement("flup").isNull());
    QVERIFY(flup.previousSiblingElement("flup", "org.example.foo").isNull());
    QVERIFY(flup.nextSiblingElement("flup", "org.example.foo").isNull());
    QVERIFY(flup.previousSiblingElement("flup", "org.example.foo2").isNull());
    QVERIFY(!flup.nextSiblingElement("flup", "org.example.foo2").isNull());

    QDomElement flup2 = flup.nextSiblingElement("flup");
    QCOMPARE(flup2.namespaceURI(), QLatin1String("org.example.foo2"));

    flup2 = foo.firstChildElement("flup", "org.example.foo2");
    QCOMPARE(flup2.namespaceURI(), QLatin1String("org.example.foo2"));
}

void tst_QDom::domNodeMapAndList()
{
    QString xml_str = QString::fromLatin1("<foo ding='dong'></foo>");

    QDomDocument doc;
    QVERIFY(doc.setContent(xml_str));

    QDomNamedNodeMap map = doc.documentElement().attributes();
    QCOMPARE(map.item(0).nodeName(), QString("ding"));
    QCOMPARE(map.item(1).nodeName(), QString()); // Make sure we don't assert

    QDomNodeList list = doc.elementsByTagName("foo");
    QCOMPARE(list.item(0).nodeName(), QString("foo"));
    QCOMPARE(list.item(1).nodeName(), QString()); // Make sure we don't assert
}

// Verifies that a default-constructed QDomDocument is null, and that calling
// any of the factory functions causes it to be non-null.
#define TEST_NULL_DOCUMENT(func) \
{ \
    QDomDocument doc; \
    QVERIFY(doc.isNull()); \
    QVERIFY(!doc.func.isNull()); \
    QVERIFY(!doc.isNull()); \
}

void tst_QDom::nullDocument()
{
    TEST_NULL_DOCUMENT(createAttribute("foo"))
    TEST_NULL_DOCUMENT(createAttributeNS("http://foo/", "bar"))
    TEST_NULL_DOCUMENT(createCDATASection("foo"))
    TEST_NULL_DOCUMENT(createComment("foo"))
    TEST_NULL_DOCUMENT(createDocumentFragment())
    TEST_NULL_DOCUMENT(createElement("foo"))
    TEST_NULL_DOCUMENT(createElementNS("http://foo/", "foo"))
    TEST_NULL_DOCUMENT(createEntityReference("foo"))
    TEST_NULL_DOCUMENT(createProcessingInstruction("foo", "bar"))
    TEST_NULL_DOCUMENT(createTextNode("foo"))
    QDomDocument doc2;
    QDomElement elt = doc2.createElement("foo");
    doc2.appendChild(elt);
    TEST_NULL_DOCUMENT(importNode(elt, true))
}

#undef TEST_NULL_DOCUMENT

void tst_QDom::invalidName_data()
{
    QTest::addColumn<QString>("in_name");
    QTest::addColumn<bool>("ok_AcceptInvalidChars");
    QTest::addColumn<bool>("ok_DropInvalidChars");
    QTest::addColumn<bool>("ok_ReturnNullNode");
    QTest::addColumn<QString>("out_name");

    QTest::newRow( "foo" )     << QString("foo")     << true  << true  << true  << QString("foo");
    QTest::newRow( "_f.o-o:" ) << QString("_f.o-o:") << true  << true  << true  << QString("_f.o-o:");
    QTest::newRow( "...:." )   << QString("...:.")   << true  << true  << false << QString(":.");
    QTest::newRow( "empty" )   << QString()          << false << false << false << QString();
    QTest::newRow( "~f~o~o~" ) << QString("~f~o~o~") << true  << true  << false << QString("foo");
    QTest::newRow( "~" )       << QString("~")       << true  << false << false << QString();
    QTest::newRow( "..." )     << QString("...")     << true  << false << false << QString();
}

void tst_QDom::invalidName()
{
    QFETCH( QString, in_name );
    QFETCH( bool, ok_AcceptInvalidChars );
    QFETCH( bool, ok_DropInvalidChars );
    QFETCH( bool, ok_ReturnNullNode );
    QFETCH( QString, out_name );

    QDomImplementation impl;
    QDomDocument doc;

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::AcceptInvalidChars);

    {
        QDomElement elt = doc.createElement(in_name);
        QDomElement elt_ns = doc.createElementNS("foo", "foo:" + in_name);
        QDomAttr attr = doc.createAttribute(in_name);
        QDomAttr attr_ns = doc.createAttributeNS("foo",  "foo:" + in_name);
        QDomEntityReference ref = doc.createEntityReference(in_name);

        QCOMPARE(!elt.isNull(), ok_AcceptInvalidChars);
        QCOMPARE(!elt_ns.isNull(), ok_AcceptInvalidChars);
        QCOMPARE(!attr.isNull(), ok_AcceptInvalidChars);
        QCOMPARE(!attr_ns.isNull(), ok_AcceptInvalidChars);
        QCOMPARE(!ref.isNull(), ok_AcceptInvalidChars);

        if (ok_AcceptInvalidChars) {
            QCOMPARE(elt.tagName(), in_name);
            QCOMPARE(elt_ns.tagName(), in_name);
            QCOMPARE(attr.name(), in_name);
            QCOMPARE(attr_ns.name(), in_name);
            QCOMPARE(ref.nodeName(), in_name);
        }
    }

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::DropInvalidChars);

    {
        QDomElement elt = doc.createElement(in_name);
        QDomElement elt_ns = doc.createElementNS("foo", "foo:" + in_name);
        QDomAttr attr = doc.createAttribute(in_name);
        QDomAttr attr_ns = doc.createAttributeNS("foo", "foo:" + in_name);
        QDomEntityReference ref = doc.createEntityReference(in_name);

        QCOMPARE(!elt.isNull(), ok_DropInvalidChars);
        QCOMPARE(!elt_ns.isNull(), ok_DropInvalidChars);
        QCOMPARE(!attr.isNull(), ok_DropInvalidChars);
        QCOMPARE(!attr_ns.isNull(), ok_DropInvalidChars);
        QCOMPARE(!ref.isNull(), ok_DropInvalidChars);

        if (ok_DropInvalidChars) {
            QCOMPARE(elt.tagName(), out_name);
            QCOMPARE(elt_ns.tagName(), out_name);
            QCOMPARE(attr.name(), out_name);
            QCOMPARE(attr_ns.name(), out_name);
            QCOMPARE(ref.nodeName(), out_name);
        }
    }

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    {
        QDomElement elt = doc.createElement(in_name);
        QDomElement elt_ns = doc.createElementNS("foo", "foo:" + in_name);
        QDomAttr attr = doc.createAttribute(in_name);
        QDomAttr attr_ns = doc.createAttributeNS("foo", "foo:" + in_name);
        QDomEntityReference ref = doc.createEntityReference(in_name);

        QCOMPARE(!elt.isNull(), ok_ReturnNullNode);
        QCOMPARE(!elt_ns.isNull(), ok_ReturnNullNode);
        QCOMPARE(!attr.isNull(), ok_ReturnNullNode);
        QCOMPARE(!attr_ns.isNull(), ok_ReturnNullNode);
        QCOMPARE(!ref.isNull(), ok_ReturnNullNode);

        if (ok_ReturnNullNode) {
            QCOMPARE(elt.tagName(), in_name);
            QCOMPARE(elt_ns.tagName(), in_name);
            QCOMPARE(attr.name(), in_name);
            QCOMPARE(attr_ns.name(), in_name);
            QCOMPARE(ref.nodeName(), in_name);
        }
    }
}

void tst_QDom::invalidQualifiedName_data()
{
    QTest::addColumn<QString>("in_name");
    QTest::addColumn<bool>("ok_AcceptInvalidChars");
    QTest::addColumn<bool>("ok_DropInvalidChars");
    QTest::addColumn<bool>("ok_ReturnNullNode");
    QTest::addColumn<QString>("out_name");

    QTest::newRow( "foo" )     << QString("foo")      << true  << true  << true  << QString("foo");
    QTest::newRow( "foo:bar" ) << QString("foo:bar")  << true  << true  << true  << QString("foo:bar");
    QTest::newRow( "bar:" )    << QString("bar:")     << false << false << false << QString();
    QTest::newRow( ":" )       << QString(":")        << false << false << false << QString();
    QTest::newRow( "empty" )   << QString()           << false << false << false << QString();
    QTest::newRow("foo:...:.") << QString("foo:...:.")<< true  << true  << false << QString("foo::.");
    QTest::newRow("foo:~")     << QString("foo:~")    << true  << false << false << QString();
    QTest::newRow("foo:.~")    << QString("foo:.~")   << true  << false << false << QString();
}

void tst_QDom::invalidQualifiedName()
{
    QFETCH( QString, in_name );
    QFETCH( bool, ok_AcceptInvalidChars );
    QFETCH( bool, ok_DropInvalidChars );
    QFETCH( bool, ok_ReturnNullNode );
    QFETCH( QString, out_name );

    QDomImplementation impl;
    QDomDocument doc;

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::AcceptInvalidChars);

    {
        QDomElement elt_ns = doc.createElementNS("foo", in_name);
        QDomAttr attr_ns = doc.createAttributeNS("foo", in_name);
        QDomDocumentType doctype = impl.createDocumentType(in_name, "foo", "bar");
        QDomDocument doc2 = impl.createDocument("foo", in_name, doctype);

        QCOMPARE(!elt_ns.isNull(), ok_AcceptInvalidChars);
        QCOMPARE(!attr_ns.isNull(), ok_AcceptInvalidChars);
        QCOMPARE(!doctype.isNull(), ok_AcceptInvalidChars);
        QCOMPARE(!doc2.isNull(), ok_AcceptInvalidChars);

        if (ok_AcceptInvalidChars) {
            QCOMPARE(elt_ns.nodeName(), in_name);
            QCOMPARE(attr_ns.nodeName(), in_name);
            QCOMPARE(doctype.name(), in_name);
            QCOMPARE(doc2.documentElement().nodeName(), in_name);
        }
    }

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::DropInvalidChars);

    {
        QDomElement elt_ns = doc.createElementNS("foo", in_name);
        QDomAttr attr_ns = doc.createAttributeNS("foo", in_name);
        QDomDocumentType doctype = impl.createDocumentType(in_name, "foo", "bar");
        QDomDocument doc2 = impl.createDocument("foo", in_name, doctype);

        QCOMPARE(!elt_ns.isNull(), ok_DropInvalidChars);
        QCOMPARE(!attr_ns.isNull(), ok_DropInvalidChars);
        QCOMPARE(!doctype.isNull(), ok_DropInvalidChars);
        QCOMPARE(!doc2.isNull(), ok_DropInvalidChars);

        if (ok_DropInvalidChars) {
            QCOMPARE(elt_ns.nodeName(), out_name);
            QCOMPARE(attr_ns.nodeName(), out_name);
            QCOMPARE(doctype.name(), out_name);
            QCOMPARE(doc2.documentElement().nodeName(), out_name);
        }
    }

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    {
        QDomElement elt_ns = doc.createElementNS("foo", in_name);
        QDomAttr attr_ns = doc.createAttributeNS("foo", in_name);
        QDomDocumentType doctype = impl.createDocumentType(in_name, "foo", "bar");
        QDomDocument doc2 = impl.createDocument("foo", in_name, doctype);

        QCOMPARE(!elt_ns.isNull(), ok_ReturnNullNode);
        QCOMPARE(!attr_ns.isNull(), ok_ReturnNullNode);
        QCOMPARE(!doctype.isNull(), ok_ReturnNullNode);
        QCOMPARE(!doc2.isNull(), ok_ReturnNullNode);

        if (ok_ReturnNullNode) {
            QCOMPARE(elt_ns.nodeName(), in_name);
            QCOMPARE(attr_ns.nodeName(), in_name);
            QCOMPARE(doctype.name(), in_name);
            QCOMPARE(doc2.documentElement().nodeName(), in_name);
        }
    }
}

void tst_QDom::invalidCharData_data()
{
    QTest::addColumn<QString>("in_text");
    QTest::addColumn<bool>("ok_AcceptInvalidChars");
    QTest::addColumn<bool>("ok_DropInvalidChars");
    QTest::addColumn<bool>("ok_ReturnNullNode");
    QTest::addColumn<QString>("out_text");

    QTest::newRow( "foo" )     << QString("foo")       << true  << true  << true  << QString("foo");
    QTest::newRow( "f<o&o" )   << QString("f<o&o")     << true  << true  << true  << QString("f<o&o");
    QTest::newRow( "empty" )   << QString()            << true  << true  << true  << QString();
    QTest::newRow("f\\x07o\\x02")<< QString("f\x07o\x02")<< true  << true  << false << QString("fo");

    const QChar pair[2] = { QChar(0xdc00), QChar(0xe000) };
    QString invalid(pair, 2);
    QTest::newRow("\\xdc00\\xe000") << invalid << true << true << false << invalid.last(1);
}

void tst_QDom::invalidCharData()
{
    QFETCH( QString, in_text );
    QFETCH( bool, ok_AcceptInvalidChars );
    QFETCH( bool, ok_DropInvalidChars );
    QFETCH( bool, ok_ReturnNullNode );
    QFETCH( QString, out_text );

    QDomDocument doc;

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::AcceptInvalidChars);

    {
        QDomText text_elt = doc.createTextNode(in_text);
        QCOMPARE(!text_elt.isNull(), ok_AcceptInvalidChars);
        if (ok_AcceptInvalidChars) {
            QCOMPARE(text_elt.nodeValue(), in_text);
        }
    }

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::DropInvalidChars);

    {
        QDomText text_elt = doc.createTextNode(in_text);
        QCOMPARE(!text_elt.isNull(), ok_DropInvalidChars);
        if (ok_DropInvalidChars) {
            QCOMPARE(text_elt.nodeValue(), out_text);
        }
    }

    QDomImplementation::setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    {
        QDomText text_elt = doc.createTextNode(in_text);
        QCOMPARE(!text_elt.isNull(), ok_ReturnNullNode);
        if (ok_ReturnNullNode) {
            QCOMPARE(text_elt.nodeValue(), in_text);
        }
    }
}

void tst_QDom::nonBMPCharacters()
{
    const auto invalidDataPolicy = QDomImplementation::invalidDataPolicy();
    auto resetInvalidDataPolicy = qScopeGuard(
            [invalidDataPolicy] { QDomImplementation::setInvalidDataPolicy(invalidDataPolicy); });
    QDomImplementation::setInvalidDataPolicy(QDomImplementation::DropInvalidChars);

    const QString input = u"<text>Supplementary Plane: 𝄞 😂 🀄 🀶 🃪 🃋</text>"_s;

    QDomDocument doc;
    QVERIFY(doc.setContent(input, QDomDocument::ParseOption::Default));
    QCOMPARE(doc.toString(-1), input);
}

void tst_QDom::roundTripAttributes() const
{
    /* Create an attribute via the QDom API with weird whitespace content. */
    QDomImplementation impl;

    QDomDocument doc(impl.createDocument("", "localName", QDomDocumentType()));

    QDomElement e(doc.documentElement());

    QString ws;
    ws.reserve(8);
    ws.append(QChar(0x20));
    ws.append(QChar(0x20));
    ws.append(QChar(0x20));
    ws.append(QChar(0xD));
    ws.append(QChar(0xA));
    ws.append(QChar(0x9));
    ws.append(QChar(0x20));
    ws.append(QChar(0x20));

    e.setAttribute("attr", ws);

    QByteArray serialized;
    QBuffer buffer(&serialized);
    buffer.open(QIODevice::WriteOnly);
    QTextStream stream(&buffer);

    doc.save(stream, 0);
    stream.flush();

    const QByteArray expected("<localName xmlns=\"\" attr=\"   &#xd;&#xa;&#x9;  \"/>\n");
    QCOMPARE(QString::fromLatin1(serialized.constData()), QString::fromLatin1(expected.constData()));
}

void tst_QDom::roundTripCDATA() const
{
    const QString input = u"<?xml version='1.0' encoding='UTF-8'?>\n"
                          "<content><![CDATA[]]></content>\n"_s;
    QDomDocument doc;
    QVERIFY(doc.setContent(input, QDomDocument::ParseOption::Default));
    QCOMPARE(doc.toString(), input);
}

void tst_QDom::normalizeEndOfLine() const
{
    QByteArray input("<a>\r\nc\rc\ra\na</a>");

    QBuffer buffer(&input);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QDomDocument doc;
    QVERIFY(doc.setContent(&buffer, QDomDocument::ParseOption::UseNamespaceProcessing));

    const QString expected(QLatin1String("<a>\nc\nc\na\na</a>"));

    // ### Qt 6: fix this, if we keep QDom at all
    QEXPECT_FAIL("", "The parser doesn't perform newline normalization. Fixing that would change behavior.", Continue);
    QCOMPARE(doc.documentElement().text(), expected);
}

void tst_QDom::normalizeAttributes() const
{
    QByteArray data("<element attribute=\"a\na\"/>");
    QBuffer buffer(&data);

    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QDomDocument doc;
    QVERIFY(doc.setContent(&buffer, QDomDocument::ParseOption::UseNamespaceProcessing));
    QCOMPARE(doc.documentElement().attribute(QLatin1String("attribute")), QString::fromLatin1("a a"));
}

void tst_QDom::serializeWeirdEOL() const
{
    QDomImplementation impl;

    QDomDocument doc(impl.createDocument("", "name", QDomDocumentType()));
    QDomElement ele(doc.documentElement());
    ele.appendChild(doc.createTextNode(QLatin1String("\r\nasd\nasd\rasd\n")));

    QByteArray output;
    QBuffer writeBuffer(&output);
    QVERIFY(writeBuffer.open(QIODevice::WriteOnly));
    QTextStream stream(&writeBuffer);

    const QByteArray expected("<name xmlns=\"\">&#xd;\nasd\nasd&#xd;asd\n</name>\n");
    doc.save(stream, 0);
    QCOMPARE(QString::fromLatin1(output.constData()), QString::fromLatin1(expected.constData()));
}

void tst_QDom::reparentAttribute() const
{
    QDomImplementation impl;
    QDomDocument doc(impl.createDocument("", "localName", QDomDocumentType()));

    QDomElement ele(doc.documentElement());
    QDomAttr attr(doc.createAttribute("localName"));
    ele.setAttributeNode(attr);

    QVERIFY(attr.ownerElement() == ele);
    QVERIFY(attr.parentNode() == ele);
}

void tst_QDom::serializeNamespaces() const
{
    const char *const input = "<doc xmlns:b='http://example.com/'>"
                              "<b:element b:name=''/>"
                              "</doc>";

    QDomDocument doc;
    QByteArray ba(input);
    QXmlStreamReader streamReader(input);
    QVERIFY(doc.setContent(&streamReader, QDomDocument::ParseOption::UseNamespaceProcessing));
    const QByteArray serialized(doc.toByteArray());

    QDomDocument doc2;
    QVERIFY(doc2.setContent(doc.toString(), QDomDocument::ParseOption::UseNamespaceProcessing));

    /* Here we test that it roundtrips. */
    QVERIFY(isDeepEqual(doc2, doc));

    QDomDocument doc3;
    QVERIFY(doc3.setContent(QString::fromLatin1(serialized.constData()),
                            QDomDocument::ParseOption::UseNamespaceProcessing));

    QVERIFY(isDeepEqual(doc3, doc));
}

void tst_QDom::flagInvalidNamespaces() const
{
    const char *const input = "<doc>"
                              "<b:element xmlns:b='http://example.com/' b:name='' xmlns:b='http://example.com/'/>"
                              "</doc>";

    QDomDocument doc;
    QVERIFY(!doc.setContent(QString::fromLatin1(input, true)));
    QVERIFY(!doc.setContent(QString::fromLatin1(input)));
}

void tst_QDom::flagUndeclaredNamespace() const
{
    /* Note, prefix 'a' is not declared. */
    const char *const input = "<a:doc xmlns:b='http://example.com/'>"
                              "<b:element b:name=''/>"
                              "</a:doc>";

    QDomDocument doc;
    QByteArray ba(input);
    QXmlStreamReader streamReader(ba);
    QVERIFY(!doc.setContent(&streamReader, QDomDocument::ParseOption::UseNamespaceProcessing));
}

void tst_QDom::indentComments() const
{
    /* We test that:
     *
     * - Whitespace is not added if a text node appears after a comment.
     * - Whitespace is not added if a text node appears before a comment.
     * - Indentation depth is linear with level depth.
     */
    const char *const input = "<e>"
                                  "<!-- A Comment -->"
                                  "<b><!-- deep --></b>"
                                  "textNode"
                                  "<!-- Another Comment -->"
                                  "<!-- Another Comment2 -->"
                                  "textNode2"
                              "</e>";
    const char *const expected = "<e>\n"
                                 "     <!-- A Comment -->\n"
                                 "     <b>\n"
                                 "          <!-- deep -->\n"
                                 "     </b>"
                                 "textNode"
                                 "<!-- Another Comment -->\n"
                                 "     <!-- Another Comment2 -->"
                                 "textNode2"
                                 "</e>\n";
    QDomDocument doc;
    QVERIFY(doc.setContent(QString::fromLatin1(input)));

    const QString serialized(doc.toString(5));

    QCOMPARE(serialized, QString::fromLatin1(expected));
}

void tst_QDom::checkLiveness() const
{
    QDomImplementation impl;

    QDomDocument doc(impl.createDocument(QString(), "doc", QDomDocumentType()));
    QDomElement ele(doc.documentElement());

    const QDomElement e1(doc.createElement("name"));
    const QDomElement e2(doc.createElement("name"));
    const QDomText t1(doc.createTextNode("content"));

    ele.appendChild(e1);
    ele.appendChild(t1);
    ele.appendChild(e2);

    const QDomNodeList children(ele.childNodes());
    QCOMPARE(children.count(), 3);

    ele.removeChild(e1);

    QCOMPARE(children.count(), 2);
    QCOMPARE(children.at(0), static_cast<const QDomNode &>(t1));
    QCOMPARE(children.at(1), static_cast<const QDomNode &>(e2));
}

void tst_QDom::reportDuplicateAttributes() const
{
    QDomDocument dd;
    bool isSuccess = bool(dd.setContent(QLatin1String("<test x=\"1\" x=\"2\"/>")));
    QVERIFY2(!isSuccess, "Duplicate attributes are well-formedness errors, and should be reported as such.");
}

void tst_QDom::namespacedAttributes() const
{
    static const char *const xml =
        "<?xml version='1.0' encoding='UTF-8' standalone='yes'?>\n"
        "<xan:td xmlns:xan=\"http://www.someurl.com/Something\" "
        "        xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
        "        xsi:schemaLocation=\"http://www.someurl.com/Something/../../xml/td.xsd\" "
        "        xmlns:xi=\"http://www.w3.org/2001/XInclude\" "
        "        xmlns=\"http://www.someurl.com/Something\">\n"
        "  <Title displayLabel='Title' >>>> SIMPLE BASIC OP - SEND - DUT AS SINK</Title>\n"
        "</xan:td>\n";

    QDomDocument one("document");
    QDomDocument::ParseResult result =
            one.setContent(QByteArray(xml), QDomDocument::ParseOption::UseNamespaceProcessing);
    QVERIFY2(result, qPrintable(result.errorMessage));
    QDomDocument two("document2");
    result = two.setContent(one.toByteArray(2), QDomDocument::ParseOption::UseNamespaceProcessing);
    QVERIFY2(result, qPrintable(result.errorMessage));

    QVERIFY(isDeepEqual(one, two));
}

void tst_QDom::appendChildFromToDocument() const
{
    QDomDocument doc;
    const QByteArray input("<e/>");

    doc.setContent(input);

    QDomDocument doc2(doc.documentElement().toDocument());
    QDomElement element = doc2.createElement("name");
    element.setAttribute("name", "value");
    doc.documentElement().appendChild(element);
}

void tst_QDom::iterateCDATA() const
{
    const QByteArray input("<e><![CDATA[data]]></e>");

    QDomDocument doc;
    QVERIFY(doc.setContent(input));
    QCOMPARE(doc.toString(), QString("<e><![CDATA[data]]></e>\n"));

    const QDomElement element(doc.documentElement());
    QVERIFY(!element.isNull());

    /* The node at element.childNodes().at(0) is not an element,
     * it's a CDATA section. */
    const QDomElement child(element.childNodes().at(0).toElement());
    QVERIFY(child.isNull());

    QVERIFY(element.childNodes().at(0).isCDATASection());
}

/*!
  \internal
  \since 4.4
  \brief This function cannot be factored into appendDocumentNode(). The
         invocation of constructors/destructors is part of triggering the bug.
 */
QDomDocument tst_QDom::generateRequest()
{
    QDomDocument doc;
    QDomElement elem = doc.createElement("test_elem");

    elem.setAttribute("name", "value");
    doc.appendChild(elem);
    return doc;
}

void tst_QDom::appendDocumentNode() const
{
    QDomDocument doc;
    QDomDocument xml = generateRequest();
    QDomElement elem = doc.createElement("document");

    doc.appendChild(elem);

    QVERIFY(!xml.isNull());
    const QString expected(QLatin1String("<document>\n<test_elem name=\"value\"/>\n</document>\n"));

    elem.appendChild(xml);
    QCOMPARE(doc.childNodes().count(), 1);
    QCOMPARE(doc.toString(0), expected);

    elem.appendChild(xml.firstChild());
    QCOMPARE(doc.childNodes().count(), 1);
    QCOMPARE(doc.toString(0), expected);
}

static const QChar umlautName[] =
{
    'a', '\xfc', 'b'
};

/*!
  \internal

  Write a german umlaut to a QByteArray, via a QTextStream.
 */
void tst_QDom::germanUmlautToByteArray() const
{
    QCOMPARE(ulong(sizeof(umlautName) /  sizeof(QChar)), ulong(3));
    const QString name(umlautName, 3);

    QDomDocument d;
    d.appendChild(d.createElement(name));
    QByteArray data;
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QTextStream ts(&buffer);
    ts << d.toString();
    buffer.close();

    QByteArray baseline("<a");

    /* http://www.fileformat.info/info/unicode/char/00FC/index.htm */
    baseline += char(0xC3);
    baseline += char(0xBC);
    baseline += "b/>\n";

    QCOMPARE(data, baseline);
}

/*!
  \internal

  Write a german umlaut to a QFile, via a QTextStream.
 */
void tst_QDom::germanUmlautToFile() const
{
    /* http://www.fileformat.info/info/unicode/char/00FC/index.htm */
    QString name(QLatin1String("german"));
    name += QChar(0xFC);
    name += QLatin1String("umlaut");
    QCOMPARE(name.size(), 13);

    QDomDocument d("test");
    d.appendChild(d.createElement(name));
    QTemporaryFile file;
    QVERIFY(file.open());
    QTextStream ts(&file);
    ts << d.toString();
    file.close();

    QFile inFile(file.fileName());
    QVERIFY(inFile.open(QIODevice::ReadOnly));

    QString baseline(QLatin1String("<!DOCTYPE test>\n<german"));
    baseline += QChar(0xFC);
    baseline += QLatin1String("umlaut/>\n");

    const QByteArray in(inFile.readAll());
    /* Check that it was wwritten out correctly. */
    QCOMPARE(in.size(), 34);
    QCOMPARE(in, baseline.toUtf8());
    inFile.close();

    /* Check that we read it in correctly with QDomDocument::setContent(). */
    QVERIFY(inFile.open(QIODevice::ReadOnly));
    QDomDocument dd;
    QVERIFY(dd.setContent(&inFile));

    QCOMPARE(dd.toString(), baseline);
}

void tst_QDom::setInvalidDataPolicy() const
{
    QDomImplementation::setInvalidDataPolicy(QDomImplementation::ReturnNullNode);
    QDomDocument doc;
    QDomElement elem = doc.createElement("invalid name");
    QVERIFY(elem.isNull());
}

void tst_QDom::crashInSetContent() const
{
    QDomImplementation::setInvalidDataPolicy(QDomImplementation::ReturnNullNode);
    QDomDocument docImport;

    QVERIFY(!docImport.setContent(QLatin1String("<a:>text</a:>"),
                                  QDomDocument::ParseOption::UseNamespaceProcessing));
    QVERIFY(docImport.setContent(QLatin1String("<?xml version=\"1.0\"?><e/>")));
}

void tst_QDom::doubleNamespaceDeclarations() const
{
    QDomDocument doc;

    QString testFile = QFINDTESTDATA("doubleNamespaces.xml");
    if (testFile.isEmpty())
        QFAIL("Cannot find test file doubleNamespaces.xml!");
    QFile file(testFile);
    QVERIFY(file.open(QIODevice::ReadOnly));

    QXmlStreamReader streamReader(&file);
    QVERIFY(doc.setContent(&streamReader, QDomDocument::ParseOption::UseNamespaceProcessing));

    QString docAsString = doc.toString(0);
    QCOMPARE(docAsString, "<a>\n<b p:c=\"\" p:d=\"\" xmlns:p=\"NS\"/>\n</a>\n");
}

void tst_QDom::setContentQXmlReaderOverload() const
{
    QDomDocument doc;
    QXmlStreamReader streamReader(QByteArray("<e/>"));
    doc.setContent(&streamReader, QDomDocument::ParseOption::UseNamespaceProcessing);
    QCOMPARE(doc.documentElement().nodeName(), QString::fromLatin1("e"));
}

void tst_QDom::cleanupTestCase() const
{
    QFile::remove("germanUmlautToFile.xml");
}

void tst_QDom::toStringWithoutNewlines() const
{
    QDomDocument doc;
    doc.setContent(QLatin1String("<doc><e/></doc>"));

    QCOMPARE(doc.toString(0), QString::fromLatin1("<doc>\n<e/>\n</doc>\n"));
    QCOMPARE(doc.toString(-1), QString::fromLatin1("<doc><e/></doc>"));
}

void tst_QDom::checkIntOverflow() const
{
    /* This test takes a *very* long time to run, so it is at best a manual
     * test. */
    return;

    /* QDom used an internal global int which overflowed. So iterate until an
     * uint wrapsaround. */
    const QString xmlMessage(QLatin1String("<test/>"));

    bool hasWrapped = false;
    for(uint i = 1; i != 0; ++i)
    {
        /* We want to exit the second time, not loop infinitely. */
        if(i == 1 && hasWrapped)
            break;
        else
            hasWrapped = true;

        QDomDocument doc;
        QVERIFY(doc.setContent(xmlMessage));

        const QDomNodeList nl(doc.elementsByTagName(QLatin1String("test")));
        QCOMPARE(nl.length(), 1);
    }
}

void tst_QDom::setContentWhitespace() const
{
    QFETCH(QString, doc);
    QFETCH(bool, expectedValidity);

    QDomDocument domDoc;

    QCOMPARE(bool(domDoc.setContent(doc)), expectedValidity);

    if(expectedValidity)
        QCOMPARE(domDoc.documentElement().nodeName(), QString::fromLatin1("e"));
}

void tst_QDom::setContentWhitespace_data() const
{
    QTest::addColumn<QString>("doc");
    QTest::addColumn<bool>("expectedValidity");

    QTest::newRow("data1") << QString::fromLatin1(" <e/>")           << true;
    QTest::newRow("data2") << QString::fromLatin1("  <e/>")          << true;
    QTest::newRow("data3") << QString::fromLatin1("   <e/>")         << true;
    QTest::newRow("data4") << QString::fromLatin1("    <e/>")        << true;
    QTest::newRow("data5") << QString::fromLatin1("\n<e/>")          << true;
    QTest::newRow("data6") << QString::fromLatin1("\n\n<e/>")        << true;
    QTest::newRow("data7") << QString::fromLatin1("\n\n\n<e/>")      << true;
    QTest::newRow("data8") << QString::fromLatin1("\n\n\n\n<e/>")    << true;
    QTest::newRow("data9") << QString::fromLatin1("\t<e/>")          << true;
    QTest::newRow("data10") << QString::fromLatin1("\t\t<e/>")        << true;
    QTest::newRow("data11") << QString::fromLatin1("\t\t\t<e/>")      << true;
    QTest::newRow("data12") << QString::fromLatin1("\t\t\t\t<e/>")    << true;

    /* With XML prolog. */
    QTest::newRow("data13") << QString::fromLatin1("<?xml version='1.0' ?><e/>")          << true;

    QTest::newRow("data14") << QString::fromLatin1(" <?xml version='1.0' ?><e/>")         << false;
    QTest::newRow("data15") << QString::fromLatin1("  <?xml version='1.0' ?><e/>")        << false;
    QTest::newRow("data16") << QString::fromLatin1("   <?xml version='1.0' ?><e/>")       << false;
    QTest::newRow("data17") << QString::fromLatin1("    <?xml version='1.0' ?><e/>")      << false;
    QTest::newRow("data18") << QString::fromLatin1("\n<?xml version='1.0' ?><e/>")        << false;
    QTest::newRow("data19") << QString::fromLatin1("\n\n<?xml version='1.0' ?><e/>")      << false;
    QTest::newRow("data20") << QString::fromLatin1("\n\n\n<?xml version='1.0' ?><e/>")    << false;
    QTest::newRow("data21") << QString::fromLatin1("\n\n\n\n<?xml version='1.0' ?><e/>")  << false;
    QTest::newRow("data22") << QString::fromLatin1("\t<?xml version='1.0' ?><e/>")        << false;
    QTest::newRow("data23") << QString::fromLatin1("\t\t<?xml version='1.0' ?><e/>")      << false;
    QTest::newRow("data24") << QString::fromLatin1("\t\t\t<?xml version='1.0' ?><e/>")    << false;
    QTest::newRow("data25") << QString::fromLatin1("\t\t\t\t<?xml version='1.0' ?><e/>")  << false;
}

void tst_QDom::setContentUnopenedQIODevice() const
{
    QByteArray data("<foo>bar</foo>");
    QBuffer buffer(&data);

    QDomDocument doc;

    QTest::ignoreMessage(QtWarningMsg,
                         "QDomDocument called with unopened QIODevice. "
                         "This will not be supported in future Qt versions.");

    // Note: the check below is expected to fail in Qt 7.
    // Fix the test and remove the obsolete code from setContent().
    QVERIFY(doc.setContent(&buffer, QDomDocument::ParseOption::UseNamespaceProcessing));
    QCOMPARE(doc.toString().trimmed(), data);
}

void tst_QDom::taskQTBUG4595_dontAssertWhenDocumentSpecifiesUnknownEncoding() const
{
    QString xmlWithUnknownEncoding("<?xml version='1.0' encoding='unknown-encoding'?>"
                                   "<foo>"
                                   " <bar>How will this sentence be handled?</bar>"
                                   "</foo>");
    QDomDocument d;
    QTest::ignoreMessage(
                QtWarningMsg,
                "QDomDocument::save(): Unsupported encoding \"unknown-encoding\" specified.");
    QVERIFY(d.setContent(xmlWithUnknownEncoding));

    QString dontAssert = d.toString(); // this should not assert
    QVERIFY(true);
}

void tst_QDom::cloneDTD_QTBUG8398() const
{
    QString dtd("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE first [\n"
                   "<!ENTITY secondFile SYSTEM 'second.xml'>\n"
                   "<!ENTITY thirdFile SYSTEM 'third.xml'>\n"
                   "]>\n"
                   "<first/>\n");
    QDomDocument domDocument;
    QVERIFY(domDocument.setContent(dtd));
    QDomDocument domDocument2 = domDocument.cloneNode(true).toDocument();

    // this string is relying on a specific QHash ordering, QTBUG-25071
    QString expected("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE first [\n"
                   "<!ENTITY thirdFile SYSTEM 'third.xml'>\n"
                   "<!ENTITY secondFile SYSTEM 'second.xml'>\n"
                   "]>\n"
                   "<first/>\n");
    QString output;
    QTextStream stream(&output);
    domDocument2.save(stream, 0);
    // check against the original string and the expected one, QTBUG-25071
    QVERIFY(output == dtd || output == expected);
}

void tst_QDom::DTDNotationDecl()
{
    QString dtd("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE first [\n"
                   "<!NOTATION gif SYSTEM 'image/gif'>\n"
                   "<!NOTATION jpeg SYSTEM 'image/jpeg'>\n"
                   "]>\n"
                   "<first/>\n");

    QDomDocument domDocument;
    QVERIFY(domDocument.setContent(dtd));

    const QDomDocumentType doctype = domDocument.doctype();
    QCOMPARE(doctype.notations().size(), 2);

    QVERIFY(doctype.namedItem(QString("gif")).isNotation());
    QCOMPARE(doctype.namedItem(QString("gif")).toNotation().systemId(), QString("image/gif"));

    QVERIFY(doctype.namedItem(QString("jpeg")).isNotation());
    QCOMPARE(doctype.namedItem(QString("jpeg")).toNotation().systemId(), QString("image/jpeg"));
}

void tst_QDom::DTDEntityDecl()
{
    QString dtd("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE first [\n"
                   "<!ENTITY secondFile SYSTEM 'second.xml'>\n"
                   "<!ENTITY logo SYSTEM \"http://www.w3c.org/logo.gif\" NDATA gif>"
                   "]>\n"
                   "<first/>\n");

    QDomDocument domDocument;
    QVERIFY(domDocument.setContent(dtd));

    const QDomDocumentType doctype = domDocument.doctype();
    QCOMPARE(doctype.entities().count(), 2);

    QVERIFY(doctype.namedItem(QString("secondFile")).isEntity());
    QCOMPARE(doctype.namedItem(QString("secondFile")).toEntity().systemId(), QString("second.xml"));
    QCOMPARE(doctype.namedItem(QString("secondFile")).toEntity().notationName(), QString());

    QVERIFY(doctype.namedItem(QString("logo")).isEntity());
    QCOMPARE(doctype.namedItem(QString("logo")).toEntity().systemId(), QString("http://www.w3c.org/logo.gif"));
    QCOMPARE(doctype.namedItem(QString("logo")).toEntity().notationName(), QString("gif"));
}

void tst_QDom::QTBUG49113_dontCrashWithNegativeIndex() const
{
    QDomDocument doc;
    QDomElement elem = doc.appendChild(doc.createElement("root")).toElement();
    QDomNode node = elem.attributes().item(-1);
    QVERIFY(node.isNull());
}

void tst_QDom::standalone()
{
    {
        QDomDocument doc;
        const QString dtd("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                    "<Test/>\n");
        doc.setContent(dtd);
        QVERIFY(doc.toString().contains("standalone=\'no\'"));
    }
    {
        QDomDocument doc;
        const QString dtd("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<Test/>\n");
        doc.setContent(dtd);
        QVERIFY(!doc.toString().contains("standalone"));
    }
    {
        QDomDocument doc;
        const QString dtd("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                    "<Test/>\n");
        doc.setContent(dtd);
        QVERIFY(doc.toString().contains("standalone=\'yes\'"));
    }
}

void tst_QDom::DTDInternalSubset() const
{
    QFETCH( QString, doc );
    QFETCH( QString, internalSubset );
    QXmlStreamReader reader(doc);
    QDomDocument document;
    QVERIFY(document.setContent(&reader, QDomDocument::ParseOption::UseNamespaceProcessing));

    QCOMPARE(document.doctype().internalSubset(), internalSubset);
}

void tst_QDom::DTDInternalSubset_data() const
{
    QTest::addColumn<QString>("doc");
    QTest::addColumn<QString>("internalSubset");

    QTest::newRow("data1") << "<?xml version='1.0'?>\n"
                              "<!DOCTYPE note SYSTEM '/[abcd].dtd'>\n"
                              "<note/>\n"
                           << "" ;

    QTest::newRow("data2") << "<?xml version='1.0'?>\n"
                              "<!DOCTYPE note PUBLIC '-/freedesktop' 'https://[abcd].dtd'>\n"
                              "<note/>\n"
                           << "" ;

    const QString internalSubset0(
                "<!-- open brackets comment [ -->\n"
                "<!-- colse brackets comment ] -->\n"
                );
    QTest::newRow("data3") << "<?xml version='1.0'?>\n"
                              "<!DOCTYPE note ["
                              + internalSubset0 +
                              "]>\n"
                              "<note/>\n"
                           << internalSubset0;

    const QString internalSubset1(
                "<!ENTITY obra '['>\n"
                "<!ENTITY cbra ']'>\n"
                );
    QTest::newRow("data4") << "<?xml version='1.0'?>\n"
                              "<!DOCTYPE note ["
                              + internalSubset1 +
                              "]>\n"
                              "<note/>\n"
                           << internalSubset1;

    QTest::newRow("data5") << "<?xml version='1.0'?>\n"
          "<!DOCTYPE note  PUBLIC '-/freedesktop' 'https://[abcd].dtd' ["
          + internalSubset0
          + "]>\n"
          "<note/>\n"
       << internalSubset0;

    QTest::newRow("data6") << "<?xml version='1.0'?>\n"
          "<!DOCTYPE note  PUBLIC '-/freedesktop' "
            "'2001:db8:130F:0000:0000:09C0:876A:130B://[abcd].dtd' ["
          + internalSubset0
          + "]>\n"
          "<note/>\n"
       << internalSubset0;
}

void tst_QDom::splitTextLeakMemory() const
{
    QDomDocument doc;
    QDomElement top = doc.createElement("top");
    QDomText text = doc.createTextNode("abcdefgh");
    top.appendChild(text);
    QDomText end = text.splitText(2);
    QCOMPARE(text.data(), "ab"_L1);
    QCOMPARE(end.data(), "cdefgh"_L1);
    // only the parent node and the document have a reference on the nodes
    QCOMPARE(text.impl->ref.loadRelaxed(), 2);
    QCOMPARE(end.impl->ref.loadRelaxed(), 2);
}

void tst_QDom::testDomListComparisonCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QDomNodeList>();
}

static QDomElement findElementByName(const QDomDocument &doc, QLatin1StringView tag)
{
    const auto list = doc.elementsByTagName(tag);
#ifdef QTEST_THROW_ON_FAIL
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.at(0).nodeType(), QDomNode::NodeType::ElementNode);
#endif
    return list.at(0).toElement();
}

void tst_QDom::testDomListComparison_data()
{
    QTest::addColumn<QDomDocument>("doc");
    QTest::addColumn<QDomNodeList>("lhs");
    QTest::addColumn<QDomNodeList>("rhs");
    QTest::addColumn<bool>("result");

    const auto xml = "<top><child1/><child2/><child3><cchild1/><cchild2/></child3></top>"_L1;

    QDomDocument doc;
    const auto result = doc.setContent(xml);
    QVERIFY2(result, result.errorMessage.toLocal8Bit().constData());

    const QDomNodeList null;
    const QDomNodeList empty = findElementByName(doc, "child1"_L1).childNodes();
    const QDomNodeList child3Children = findElementByName(doc, "child3"_L1).childNodes();
    const QDomNodeList topChildren = findElementByName(doc, "top"_L1).childNodes();

#define ROW(lhs, rhs, res) \
    QTest::addRow("%s <> %s", #lhs, #rhs) << doc << lhs << rhs << bool(res)

    ROW(null, null, true);
    ROW(empty, empty, true);
    ROW(child3Children, child3Children, true);
    ROW(topChildren, topChildren, true);

    ROW(null, empty, false);
    ROW(empty, child3Children, false);
    ROW(child3Children, topChildren, false);
    ROW(topChildren, null, false);
#undef ROW
}

void tst_QDom::testDomListComparison()
{
    [[maybe_unused]]
    QFETCH(const QDomDocument, doc);
    QFETCH(const QDomNodeList, lhs);
    QFETCH(const QDomNodeList, rhs);
    QFETCH(const bool, result);

    QT_TEST_EQUALITY_OPS(lhs, rhs, result);
}

QTEST_MAIN(tst_QDom)
#include "tst_qdom.moc"
