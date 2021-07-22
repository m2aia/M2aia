/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <algorithm>
#include <boost/progress.hpp>
#include <itkImage.h>
#include <itksys/SystemTools.hxx>
#include <m2ImzMLSpectrumImage.h>
#include <mitkCommandLineParser.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mutex>
#include <numeric>
#include <stdlib.h>

std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  std::map<std::string, us::Any> argsMap;
  std::string params = "";

  if (argc > 1)
  {
    argsMap = CommandlineParsing(argc, argv);
    auto ifs = std::ifstream(argsMap["parameterfile"].ToString());
    params = std::string(std::istreambuf_iterator<char>{ifs}, {});
  }

  using namespace std::string_literals;
  std::map<std::string, std::string> pMap;
  const auto bsc_s = m2::Find(params, "baseline-correction", "None"s, pMap);
  const auto bsc_hw = m2::Find(params, "baseline-correction-hw", int(50), pMap);
  const auto sm_s = m2::Find(params, "smoothing", "None"s, pMap);
  const auto sm_hw = m2::Find(params, "smoothing-hw", int(2), pMap);
  const auto norm = m2::Find(params, "normalization", "None"s, pMap);
  const auto y_output_type = m2::Find(params, "y-type", "Float"s, pMap);
  const auto x_output_type = m2::Find(params, "x-type", "Float"s, pMap);

  if (argsMap.find("parameterfile") == argsMap.end())
  {
    try
    {
      using namespace itksys;
      auto cwd = SystemTools::GetCurrentWorkingDirectory();
      auto path = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({cwd, "/m2ParmameterFile.txt.sample"}));
      std::ofstream ofs(path);
      for (auto kv : pMap)
      {
        ofs << "(" << kv.first << ") " << kv.second << "\n";
      }
      MITK_INFO << "A dummy parameter file was written to " << path;
      return 0;
    }
    catch (std::exception &e)
    {
      MITK_INFO << "Error on writing a dummy parameter file! " << e.what();
      return 2;
    }
  }

  auto image = mitk::IOUtil::Load(argsMap["input"].ToString()).front();

  for (auto kv : argsMap)
  {
    MITK_INFO << kv.first << " " << kv.second.ToString();
  }
  if (auto sImage = dynamic_cast<m2::SpectrumImageBase *>(image.GetPointer()))
  {
    if (sImage->GetImportMode() != m2::SpectrumFormatType::ContinuousProfile)
    {
      MITK_ERROR << "Only imzML files in continuous profile mode are accepted for processing!";
      return 1;
    }

    sImage->SetBaselineCorrectionStrategy(static_cast<m2::BaselineCorrectionType>(m2::SIGNAL_MAPPINGS.at(bsc_s)));
    sImage->SetBaseLineCorrectionHalfWindowSize(bsc_hw);

    sImage->SetSmoothingStrategy(static_cast<m2::SmoothingType>(m2::SIGNAL_MAPPINGS.at(sm_s)));
    sImage->SetSmoothingHalfWindowSize(sm_hw);

    sImage->SetNormalizationStrategy(static_cast<m2::NormalizationStrategyType>(m2::SIGNAL_MAPPINGS.at(norm)));
    sImage->InitializeImageAccess();
    
    sImage->SetExportMode(m2::SpectrumFormatType::ContinuousProfile);
    sImage->SetYOutputType(static_cast<m2::NumericType>(m2::CORE_MAPPINGS.at(y_output_type)));
    sImage->SetXOutputType(static_cast<m2::NumericType>(m2::CORE_MAPPINGS.at(x_output_type)));

    mitk::IOUtil::Save(sImage, argsMap["output"].ToString());
  }
}

std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[])
{
  mitkCommandLineParser parser;
  parser.setArgumentPrefix("--", "-");
  // required params
  parser.addArgument("input",
                     "i",
                     mitkCommandLineParser::Image,
                     "Input imzML Image",
                     "Path to the input imzML",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("parameterfile",
                     "p",
                     mitkCommandLineParser::File,
                     "Parameter file",
                     "A dummy parameter file can be generated by calling the app without any arguments.",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("output",
                     "o",
                     mitkCommandLineParser::Image,
                     "Output Image",
                     "Path to the output image path",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Output);

  // Miniapp Infos
  parser.setCategory("M2aia Tools");
  parser.setTitle("Basic signal processing");
  parser.setDescription(
    "Reads an imzML file and apply signal processing. https://m2aia.de (https://bio.tools/m2aia)");
  parser.setContributor("Jonas Cordes");

  auto parsedArgs = parser.parseArguments(argc, argv);
  if (parsedArgs.size() == 0)
  {
    exit(EXIT_SUCCESS);
  }

    MITK_INFO << "H";
  return parsedArgs;
}
