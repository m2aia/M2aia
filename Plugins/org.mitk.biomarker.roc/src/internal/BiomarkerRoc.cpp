/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "BiomarkerRoc.h"

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <berryPlatform.h>
#include <berryPlatformUI.h>

// Qmitk
#include <QmitkAbstractNodeSelectionWidget.h>

// Qt
#include <QDialog>
#include <QLineSeries>
#include <QMessageBox>
#include <QValueAxis>
#include <QtCharts/QChartView>

// mitk
#include <mitkImage.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

// m2aia
#include <m2ImzMLSpectrumImage.h>
#include <m2ReceiverOperatingCharacteristic.h>
#include <m2SpectrumImageBase.h>
#include <m2Peak.h>

// std
#include <array>
#include <tuple>

// for logging purposes
#define ROC_SIG "[BiomarkerRoc] "

struct Timer
{
  Timer() { time = std::chrono::high_resolution_clock::now(); }
  ~Timer()
  {
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count();
    auto end = std::chrono::time_point_cast<std::chrono::microseconds>(stop_time).time_since_epoch().count();
    auto duration = end - start;
    MITK_INFO << ROC_SIG << "execution took " << duration << " microseconds";
  }

private:
  std::chrono::_V2::system_clock::time_point time;
};

const std::string BiomarkerRoc::VIEW_ID = "org.mitk.views.biomarkerrocanalysis";

BiomarkerRoc::BiomarkerRoc() : m_Image(nullptr), m_MaskData(nullptr), m_ImageData(nullptr), m_ImageDataSize(0) {}

void BiomarkerRoc::SetFocus()
{
  m_Controls.label->setFocus();
}

void BiomarkerRoc::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);
  m_Controls.tableWidget->setVisible(false);
  m_Controls.tableWidget->setColumnCount(2);
  m_Controls.tableWidget->setRowCount(0);
  m_Controls.tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("m/z"));
  m_Controls.tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("AUC"));
  m_Controls.chartView->setVisible(false);
  m_Controls.image->SetDataStorage(GetDataStorage());
  m_Controls.image->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.selection->SetDataStorage(GetDataStorage());
  m_Controls.selection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::LabelSetImage>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.image->SetSelectionIsOptional(false);
  m_Controls.image->SetInvalidInfo("Choose image");
  m_Controls.image->SetAutoSelectNewNodes(true);
  m_Controls.image->SetPopUpTitel("Select image");
  m_Controls.image->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  m_Controls.selection->SetSelectionIsOptional(false);
  m_Controls.selection->SetInvalidInfo("Choose selection");
  m_Controls.selection->SetAutoSelectNewNodes(true);
  m_Controls.selection->SetPopUpTitel("Select selection");
  m_Controls.selection->SetPopUpHint(
    "Choose the selection you want to work with. This can be any currently opened selection.");
  connect(m_Controls.buttonCalc, &QPushButton::clicked, this, &BiomarkerRoc::OnButtonCalcPressed);
  connect(m_Controls.buttonChart, &QPushButton::clicked, this, &BiomarkerRoc::OnButtonRenderChartPressed);
  connect(m_Controls.buttonOpenPeakPickingView, &QCommandLinkButton::clicked, this, []() {
    try
    {
      if (auto platform = berry::PlatformUI::GetWorkbench())
        if (auto workbench = platform->GetActiveWorkbenchWindow())
          if (auto page = workbench->GetActivePage())
            page->ShowView("org.mitk.views.m2.PeakPicking");
    }
    catch (berry::PartInitException &e)
    {
      BERRY_ERROR << "Error: " << e.what() << std::endl;
    }
  });
}

void BiomarkerRoc::OnButtonCalcPressed()
{
  // initialize
  auto imageNode = m_Controls.image->GetSelectedNode();
  auto maskNode = m_Controls.selection->GetSelectedNode();
  if (!imageNode || !maskNode)
    return;
  auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData());
  auto mask = dynamic_cast<mitk::Image *>(maskNode->GetData());
  m_Image = mitk::Image::New(); // image to which the mask will be applied to
  m_Image->Initialize(originalImage);
  mitk::ImageReadAccessor readAccessorMask(mask);
  m_MaskData = static_cast<const mitk::Label::PixelType *>(readAccessorMask.GetData());
  auto peaks = originalImage->GetPeaks();
  for (auto& peak : peaks)
  {
    double mz = peak.GetX();
    originalImage->GetImage(mz, m_Tolerance, mask, m_Image);
    mitk::ImageReadAccessor imagereader(m_Image);
    auto dims = m_Image->GetDimensions();
    m_ImageDataSize = dims[0] * dims[1] * dims[2];
    m_ImageData = static_cast<const double*>(imagereader.GetData());

    auto tuple = GetLabeledMz();
    std::vector<std::tuple<double, bool>> D;
    size_t P, N;
    std::tie(D, P, N) = tuple;
    
    double auc = m2::ReceiverOperatorCharacteristic::DoRocAnalysis(D.begin(), D.end(), P, N);
    AddToTable(mz, auc);
  }
  m_Controls.tableWidget->setVisible(true);
}

void BiomarkerRoc::OnButtonRenderChartPressed()
{
  double mz = m_Controls.mzValue->value();
  RefreshImageWithNewMz(mz);
  // get P and N
  auto tuple = GetLabeledMz();
  std::vector<std::tuple<double, bool>> D;
  size_t P, N;
  std::tie(D, P, N) = tuple;
  auto auc_tuple = m2::ReceiverOperatorCharacteristic::DoRocAnalysisSlow(D.begin(), D.end(), P, N);
  double AUC;
  std::vector<std::tuple<double, double>> TrueRates;
  std::tie(TrueRates, AUC) = auc_tuple;

  char auc[16] = {0};
  snprintf(auc, 15, "%s%lf", "AUC: ", AUC);
  m_Controls.labelAuc->setText(auc);

  auto serie = new QtCharts::QLineSeries();
  for (size_t idx = 0; idx < TrueRates.size(); ++idx)
  {
    double tpr, fpr;
    std::tie(fpr, tpr) = TrueRates[idx];
    serie->append(fpr, tpr);
    // MITK_INFO << ROC_SIG << "[" << idx << "] FPR: " << fpr << ", TPR: " << tpr;
  }
  auto chart = new QtCharts::QChart();
  chart->addSeries(serie);
  auto axisX = new QValueAxis();
  axisX->setMin(0);
  axisX->setMax(1);
  auto axisY = new QValueAxis();
  axisY->setMin(0);
  axisY->setMax(1);
  chart->addAxis(axisX, Qt::AlignBottom);
  chart->addAxis(axisY, Qt::AlignLeft);
  chart->setTheme(QtCharts::QChart::ChartTheme::ChartThemeDark);
  m_Controls.chartView->setChart(chart);
  m_Controls.chartView->update();
  m_Controls.chartView->setVisible(true);
}

void BiomarkerRoc::AddToTable(double mz, double auc)
{
  int size = m_Controls.tableWidget->rowCount();
  m_Controls.tableWidget->setRowCount(size + 1);
  char mztext[16] = {0};
  snprintf(mztext, 16, "%lf", mz);
  auto mzlabel = new QLabel();
  mzlabel->setText(mztext);
  m_Controls.tableWidget->setCellWidget(size, 0, mzlabel);
  char auctext[16] = {0};
  snprintf(auctext, 16, "%lf", auc);
  auto auclabel = new QLabel();
  auclabel->setText(auctext);
  m_Controls.tableWidget->setCellWidget(size, 1, auclabel);
}

void BiomarkerRoc::RefreshImageWithNewMz(double mz)
{
  auto imageNode = m_Controls.image->GetSelectedNode();
  auto maskNode = m_Controls.selection->GetSelectedNode();
  auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData());
  auto mask = dynamic_cast<mitk::Image *>(maskNode->GetData());
  if (!m_Image)
    m_Image = mitk::Image::New();
  m_Image->Initialize(originalImage);
  originalImage->GetImage(mz, m_Tolerance, mask, m_Image); // write in m_Image
  // get read access to the images
  mitk::ImageReadAccessor readAccessorImage(m_Image);
  mitk::ImageReadAccessor readAccessorMask(mask);
  m_MaskData = static_cast<const mitk::Label::PixelType*>(readAccessorMask.GetData());
  m_ImageData = static_cast<const double *>(readAccessorImage.GetData());
  auto dims = m_Image->GetDimensions();
  m_ImageDataSize = dims[0] * dims[1] * dims[2];
}

std::tuple<std::vector<std::tuple<double, bool>>, size_t, size_t> BiomarkerRoc::GetLabeledMz()
{
  // prepare data for ROC
  size_t P = 0; //NUM TUMOR
  size_t N = 0; //NUM NONTUMOR
  std::vector<double> A, B;
  A.reserve(m_ImageDataSize);
  B.reserve(m_ImageDataSize);
  auto maskIter = m_MaskData;
  for (auto imageIter = m_ImageData; imageIter != m_ImageData + m_ImageDataSize; ++maskIter, ++imageIter)
  {
    if (*maskIter == 1)
    {
      A.push_back(*imageIter); // positives
      ++P;
    }
    else if (*maskIter == 2)
    {
      B.push_back(*imageIter); // negatives
      ++N;
    }
  }
  A.shrink_to_fit();
  B.shrink_to_fit();
  constexpr const bool TUMOR = true;
  constexpr const bool NONTUMOR = false;  
  std::vector<std::tuple<double, bool>> D;
  D.reserve(P + N);
  for (size_t i = 0; i < A.size(); ++i)
    D.push_back(std::make_tuple(A[i], TUMOR));
  for (size_t i = 0; i < B.size(); ++i)
    D.push_back(std::make_tuple(B[i], NONTUMOR));
  std::sort(D.begin(), D.end(), [](const std::tuple<double, bool>& a, const std::tuple<double, bool>& b)
  {
    double da, db;
    bool ba, bb;
    std::tie(da, ba) = a;
    std::tie(db, bb) = b;
    return da < db;
  });
  return std::make_tuple(D, P, N);
}
/*
  if (false) // as of now this is considered a debug feature
  {
    // add new image to DataManager
    auto dataNode = mitk::DataNode::New();
    dataNode->SetData(m_Image);
    dataNode->SetName("Biomarker Roc data");
    GetDataStorage()->Add(dataNode, m_Controls.image->GetSelectedNode());
  }
*/