/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkSingleNodeSelectionWidget.h"

#include <berryQtStyleManager.h>

#include "mitkNodePredicateFunction.h"
#include "mitkNodePredicateAnd.h"

#include <QMouseEvent>

#include "QmitkNodeSelectionDialog.h"
#include "QmitkNodeDetailsDialog.h"

QmitkSingleNodeSelectionWidget::QmitkSingleNodeSelectionWidget(QWidget* parent)
  : QmitkAbstractNodeSelectionWidget(parent)
  , m_AutoSelectNodes(true)
{
  m_Controls.setupUi(this);

  m_Controls.btnSelect->installEventFilter(this);
  m_Controls.btnSelect->setVisible(true);
  m_Controls.btnClear->setVisible(false);

  m_Controls.btnClear->setIcon(berry::QtStyleManager::ThemeIcon(QStringLiteral(":/org.mitk.gui.qt.common/times.svg")));

  this->UpdateInfo();

  connect(m_Controls.btnClear, SIGNAL(clicked(bool)), this, SLOT(OnClearSelection()));
}

void QmitkSingleNodeSelectionWidget::ReviseSelectionChanged(const NodeList& oldInternalSelection, NodeList& newInternalSelection)
{
  if (newInternalSelection.empty())
  {
    if (m_AutoSelectNodes)
    {
      auto autoSelectedNode = this->DetermineAutoSelectNode(oldInternalSelection);

      if (autoSelectedNode.IsNotNull())
      {
        newInternalSelection.append(autoSelectedNode);
      }
    }
  }
  else if (newInternalSelection.size()>1)
  {
    //this widget only allows one internal selected node.
    newInternalSelection = { newInternalSelection.front() };
  }
}

void QmitkSingleNodeSelectionWidget::OnClearSelection()
{
  if (m_IsOptional)
  {
    this->SetCurrentSelection({});
  }

  this->UpdateInfo();
}

mitk::DataNode::Pointer QmitkSingleNodeSelectionWidget::GetSelectedNode() const
{
  mitk::DataNode::Pointer result;

  auto selection = GetCurrentInternalSelection();
  if (!selection.empty())
  {
    result = selection.front();
  }
  return result;
}

bool QmitkSingleNodeSelectionWidget::eventFilter(QObject *obj, QEvent *ev)
{
  if (obj == m_Controls.btnSelect)
  {
    if (ev->type() == QEvent::MouseButtonRelease)
    {
      auto mouseEv = dynamic_cast<QMouseEvent*>(ev);
      if (!mouseEv)
      {
        return false;
      }

      if (mouseEv->button() == Qt::LeftButton)
      {
        if (this->isEnabled())
        {
          this->EditSelection();
          return true;
        }
      }
      else
      {
        auto selection = this->CompileEmitSelection();
        if (!selection.empty())
        {
          QmitkNodeDetailsDialog infoDialog(selection, this);
          infoDialog.exec();
          return true;
        }
      }
    }
  }

  return false;
}

void QmitkSingleNodeSelectionWidget::EditSelection()
{
  QmitkNodeSelectionDialog* dialog = new QmitkNodeSelectionDialog(this, m_PopUpTitel, m_PopUpHint);

  dialog->SetDataStorage(m_DataStorage.Lock());
  dialog->SetNodePredicate(m_NodePredicate);
  dialog->SetCurrentSelection(this->GetCurrentInternalSelection());
  dialog->SetSelectOnlyVisibleNodes(m_SelectOnlyVisibleNodes);
  dialog->SetSelectionMode(QAbstractItemView::SingleSelection);

  m_Controls.btnSelect->setChecked(true);

  if (dialog->exec())
  {
    this->HandleChangeOfInternalSelection(dialog->GetSelectedNodes());
  }

  m_Controls.btnSelect->setChecked(false);

  delete dialog;
}

void QmitkSingleNodeSelectionWidget::UpdateInfo()
{
  if (this->GetSelectedNode().IsNull())
  {
    if (m_IsOptional)
    {
      m_Controls.btnSelect->SetNodeInfo(m_EmptyInfo);
    }
    else
    {
      m_Controls.btnSelect->SetNodeInfo(m_InvalidInfo);
    }
    m_Controls.btnSelect->SetSelectionIsOptional(m_IsOptional);
    m_Controls.btnClear->setVisible(false);
  }
  else
  {
    m_Controls.btnClear->setVisible(m_IsOptional);
  }

  m_Controls.btnSelect->SetSelectedNode(this->GetSelectedNode());
}

void QmitkSingleNodeSelectionWidget::SetCurrentSelectedNode(mitk::DataNode* selectedNode)
{
  NodeList selection;
  if (selectedNode)
  {
    selection.append(selectedNode);
  }
  this->SetCurrentSelection(selection);
}

void QmitkSingleNodeSelectionWidget::OnDataStorageChanged()
{
  this->AutoSelectNodes();
}

void QmitkSingleNodeSelectionWidget::OnNodeAddedToStorage(const mitk::DataNode* /*node*/)
{
  this->AutoSelectNodes();
}

bool QmitkSingleNodeSelectionWidget::GetAutoSelectNewNodes() const
{
  return m_AutoSelectNodes;
}

void QmitkSingleNodeSelectionWidget::SetAutoSelectNewNodes(bool autoSelect)
{
  m_AutoSelectNodes = autoSelect;
  this->AutoSelectNodes();
}

void QmitkSingleNodeSelectionWidget::AutoSelectNodes()
{
  if (this->GetSelectedNode().IsNull() && m_AutoSelectNodes)
  {
    auto autoNode = this->DetermineAutoSelectNode();

    if (autoNode.IsNotNull())
    {
      this->HandleChangeOfInternalSelection({ autoNode });
    }
  }
}

void QmitkSingleNodeSelectionWidget::InitSurfaceSelector(mitk::DataStorage *aDataStorage)
{
  this->SetDataStorage(aDataStorage);
  this->SetNodePredicate(mitk::NodePredicateAnd::New(
    mitk::TNodePredicateDataType<mitk::Surface>::New(),
    mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(mitk::NodePredicateProperty::New("helper object"),
                                                           mitk::NodePredicateProperty::New("hidden object")))));

  this->SetSelectionIsOptional(true);
  this->SetAutoSelectNewNodes(true);
  this->SetEmptyInfo(QString("Please select a surface"));
  this->SetPopUpTitel(QString("Select surface"));
}

void QmitkSingleNodeSelectionWidget::InitPointSetSelector(mitk::DataStorage *aDataStorage) 
{
  this->SetDataStorage(aDataStorage);
  this->SetNodePredicate(mitk::NodePredicateAnd::New(
    mitk::TNodePredicateDataType<mitk::PointSet>::New(),
    mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(mitk::NodePredicateProperty::New("helper object"),
                                                           mitk::NodePredicateProperty::New("hidden object")))));

  this->SetSelectionIsOptional(true);
  this->SetAutoSelectNewNodes(true);
  this->SetEmptyInfo(QString("Please select a point set"));
  this->SetPopUpTitel(QString("Select point set"));
}

//void QmitkSingleNodeSelectionWidget::InitImageSelector(mitk::DataStorage *aDataStorage) 
//{
//  /*this->SetDataStorage(aDataStorage);
//  this->SetNodePredicate(mitk::NodePredicateAnd::New(
//    mitk::TNodePredicateDataType<mitk::Image>::New(),
//    mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(mitk::NodePredicateProperty::New("helper object"),
//                                                           mitk::NodePredicateProperty::New("hidden object")))));
//
//  this->SetSelectionIsOptional(true);
//  this->SetAutoSelectNewNodes(true);
//  this->SetEmptyInfo(QString("Please select a image"));
//  this->SetPopUpTitel(QString("Select image"));*/
//}

mitk::DataNode::Pointer QmitkSingleNodeSelectionWidget::DetermineAutoSelectNode(const NodeList& ignoreNodes)
{
  mitk::DataNode::Pointer result;
  auto storage = m_DataStorage.Lock();
  if (storage.IsNotNull())
  {
    auto ignoreCheck = [ignoreNodes](const mitk::DataNode * node)
    {
      bool result = true;
      for (const auto& ignoreNode : ignoreNodes)
      {
        if (node == ignoreNode)
        {
          result = false;
          break;
        }
      }
      return result;
    };

    mitk::NodePredicateFunction::Pointer isNotIgnoredNode = mitk::NodePredicateFunction::New(ignoreCheck);
    mitk::NodePredicateBase::Pointer predicate = isNotIgnoredNode.GetPointer();

    if (m_NodePredicate.IsNotNull())
    {
      predicate = mitk::NodePredicateAnd::New(m_NodePredicate.GetPointer(), predicate.GetPointer()).GetPointer();
    }

    result = storage->GetNode(predicate);
  }
  return result;
}
