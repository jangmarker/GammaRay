/*
  modelinspectortest.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Volker Krause <volker.krause@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <probe/hooks.h>
#include <probe/probecreator.h>
#include <core/probe.h>
#include <common/objectbroker.h>
#include <common/objectmodel.h>
#include <common/objectid.h>

#include <3rdparty/qt/modeltest.h>

#include <QtTest/qtest.h>

#include <QItemSelectionModel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QObject>

using namespace GammaRay;

class ModelInspectorTest : public QObject
{
    Q_OBJECT
private:
    void createProbe()
    {
        Hooks::installHooks();
        Probe::startupHookReceived();
        new ProbeCreator(ProbeCreator::Create);
        QTest::qWait(1); // event loop re-entry
    }

    QModelIndex indexForName(QAbstractItemModel *model, const QString &name)
    {
        const auto matchResult = model->match(model->index(0, 0), Qt::DisplayRole, name, 1, Qt::MatchExactly | Qt::MatchRecursive);
        if (matchResult.size() < 1)
            return QModelIndex();
        const auto idx = matchResult.at(0);
        Q_ASSERT(idx.isValid());
        return idx;
    }

private slots:
    void testModelModel()
    {
        createProbe();

        auto targetModel = new QStandardItemModel;
        targetModel->setObjectName("targetModel");
        QTest::qWait(1); // trigger model inspector plugin loading

        auto modelModel = ObjectBroker::model("com.kdab.GammaRay.ModelModel");
        QVERIFY(modelModel);
        ModelTest modelModelTester(modelModel);
        QVERIFY(modelModel->rowCount() >= 1); // can contain the QEmptyModel instance too
        int topRowCount = modelModel->rowCount();

        auto targetModelIdx = indexForName(modelModel, QLatin1String("targetModel"));
        QVERIFY(targetModelIdx.isValid());
        QCOMPARE(targetModelIdx.data(ObjectModel::ObjectIdRole).value<ObjectId>(), ObjectId(targetModel));
        QCOMPARE(targetModelIdx.sibling(targetModelIdx.row(), 1).data().toString(), QLatin1String("QStandardItemModel"));
        QVERIFY(!modelModel->hasChildren(targetModelIdx));

        // proxy with source
        auto targetProxy = new QSortFilterProxyModel;
        targetProxy->setSourceModel(targetModel);
        QTest::qWait(1);

        targetModelIdx = indexForName(modelModel, QLatin1String("targetModel")); // re-lookup, due to model reset
        QVERIFY(targetModelIdx.isValid());
        QVERIFY(modelModel->hasChildren(targetModelIdx));
        QCOMPARE(modelModel->rowCount(), topRowCount);

        delete targetProxy;
        QTest::qWait(1);

        targetModelIdx = indexForName(modelModel, QLatin1String("targetModel")); // re-lookup, due to model reset
        QVERIFY(targetModelIdx.isValid());
        QVERIFY(!modelModel->hasChildren(targetModelIdx));
        QCOMPARE(modelModel->rowCount(), topRowCount);

        // proxy with source added/reset later
        targetProxy = new QSortFilterProxyModel;
        targetProxy->setObjectName("targetProxy");
        QTest::qWait(1);
        QCOMPARE(modelModel->rowCount(), topRowCount + 1);
        auto proxyIdx = indexForName(modelModel, "targetProxy");
        QVERIFY(proxyIdx.isValid());
        QVERIFY(!proxyIdx.parent().isValid());

        targetProxy->setSourceModel(targetModel);
        QCOMPARE(modelModel->rowCount(), topRowCount);
        proxyIdx = indexForName(modelModel, "targetProxy");
        QVERIFY(proxyIdx.isValid());
        QVERIFY(proxyIdx.parent().isValid());

        targetProxy->setSourceModel(Q_NULLPTR);
        QCOMPARE(modelModel->rowCount(), topRowCount + 1);
        proxyIdx = indexForName(modelModel, "targetProxy");
        QVERIFY(proxyIdx.isValid());
        QVERIFY(!proxyIdx.parent().isValid());

        delete targetProxy;
        QTest::qWait(1);
        QCOMPARE(modelModel->rowCount(), topRowCount);

        // 2 element proxy chain
        targetProxy = new QSortFilterProxyModel;
        targetProxy->setObjectName("targetProxy");
        QTest::qWait(1);
        auto targetProxy2 = new QSortFilterProxyModel(targetProxy);
        targetProxy2->setObjectName("targetProxy2");
        QTest::qWait(1);
        targetProxy2->setSourceModel(targetProxy);
        targetProxy->setSourceModel(targetModel);
        QCOMPARE(modelModel->rowCount(), topRowCount);
        proxyIdx = indexForName(modelModel, "targetProxy2");
        QVERIFY(proxyIdx.isValid());
        auto idx = proxyIdx.parent();
        QVERIFY(idx.isValid());
        QCOMPARE(modelModel->rowCount(idx), 1);
        QCOMPARE(idx.data().toString(), QLatin1String("targetProxy"));
        idx = idx.parent();
        QVERIFY(idx.isValid());
        QVERIFY(!idx.parent().isValid());
        QCOMPARE(modelModel->rowCount(idx), 1);
        QCOMPARE(idx.data().toString(), QLatin1String("targetModel"));

        delete targetProxy;
        QTest::qWait(1);
        QCOMPARE(modelModel->rowCount(), topRowCount);

        // QAIM removal
        delete targetModel;
        QTest::qWait(1);

        targetModelIdx = indexForName(modelModel, QLatin1String("targetModel"));
        QVERIFY(!targetModelIdx.isValid());
        QCOMPARE(modelModel->rowCount(), topRowCount - 1);
    }
};

QTEST_MAIN(ModelInspectorTest)

#include "modelinspectortest.moc"