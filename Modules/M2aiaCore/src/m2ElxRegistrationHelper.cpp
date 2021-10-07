#include <clocale>
#include <m2ElxDefaultParameterFiles.h>
#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>
#include <mitkImage2DToImage3DSliceFilter.h>
#include <mitkImage3DSliceToImage2DFilter.h>

m2::ElxRegistrationHelper::~ElxRegistrationHelper() {}

bool m2::ElxRegistrationHelper::CheckDimensions(const mitk::Image *image) const
{
  const auto dims = image->GetDimension();
  // const auto sizeZ = image->GetDimensions()[2];

  return dims == 3 || dims == 2;
}

mitk::Image::Pointer m2::ElxRegistrationHelper::GetSlice2DData(const mitk::Image *image) const
{
  if (image)
  {
    const auto dims = image->GetDimension();
    const auto sizeZ = image->GetDimensions()[2];

    if (dims == 3 && sizeZ == 1)
    {
      auto filter = mitk::Image3DSliceToImage2DFilter::New();
      filter->SetInput(image);
      filter->Update();
      return {filter->GetOutput()};
    }
    else if (dims == 2)
    {
      return const_cast<mitk::Image *>(image);
    }
    else if (dims == 3 && sizeZ > 1)
    {
      return const_cast<mitk::Image *>(image);
    }
    else if (dims == 4)
    {
      mitkThrow() << "3D+t Images are not supported!";
    }
  }
  mitkThrow() << "Image data is null!";
}

mitk::Image::Pointer m2::ElxRegistrationHelper::GetSlice3DData(const mitk::Image *image) const
{
  if (image)
  {
    if (image->GetDimension() == 2)
    {
      auto filter = mitk::Image2DToImage3DSliceFilter::New();
      filter->SetInput(image);
      filter->Update();
      return {filter->GetOutput()};
    }
    else
    {
      return const_cast<mitk::Image *>(image);
    }
  }
  else
  {
    mitkThrow() << "Image data is null!";
  }
}

void m2::ElxRegistrationHelper::SetPointData(mitk::PointSet *fixed, mitk::PointSet *moving)
{
  if (fixed == nullptr || moving == nullptr)
  {
    MITK_WARN << "Fixed pointset is [" << fixed << "]; moving pointset is [" << moving << "]";
    MITK_WARN << "No pointsets are used.";
    m_UsePointsForRegistration = false;
  }
  else
  {
    m_FixedPoints = fixed;
    m_MovingPoints = moving;
    m_UsePointsForRegistration = true;
  }
}

void m2::ElxRegistrationHelper::SetMaskData(mitk::Image *fixed, mitk::Image *moving)
{
  if (fixed == nullptr || moving == nullptr)
  {
    MITK_WARN << "Can not proceed: fixed mask is [" << fixed << "]; moving mask is [" << moving << "]";
    return;
  }

  if (!CheckDimensions(fixed) && !CheckDimensions(moving))
  {
    MITK_ERROR << "Fixed mask image [" << m2::ElxUtil::GetShape(fixed) << "] and moving mask image ["
               << m2::ElxUtil::GetShape(moving) << "]! This is yet not implemented.\n"
               << "Shape has to be [NxMx1]";
    return;
  }

  m_FixedMask = fixed;
  m_MovingMask = moving;

  // if images were already set, check if geometries fit
  if (m_FixedImage && m_MovingImage)
  {
    if (!(mitk::Equal(*m_FixedImage->GetGeometry(), *m_FixedMask->GetGeometry()) &&
          mitk::Equal(*m_MovingImage->GetGeometry(), *m_MovingMask->GetGeometry())))
    {
      MITK_ERROR << "Fixed image [" << m2::ElxUtil::GetShape(m_FixedImage) << "] and fixed mask image ["
                 << m2::ElxUtil::GetShape(m_FixedMask) << "].\n"
                 << "Moving image [" << m2::ElxUtil::GetShape(m_MovingImage) << "] and moving mask image ["
                 << m2::ElxUtil::GetShape(m_MovingMask) << "].\n"
                 << "Image geometries of mask image and image have to be equal!";
    }
  }

  m_UseMasksForRegistration = true;
}

void m2::ElxRegistrationHelper::SetImageData(mitk::Image *fixed, mitk::Image *moving)
{
  if (fixed == nullptr || moving == nullptr)
    mitkThrow() << "Can not proceed: fixed is [" << fixed << "]; moving is [" << moving << "]";

  if (!CheckDimensions(fixed) && !CheckDimensions(moving))
    mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(fixed) << "] and moving image ["
                << m2::ElxUtil::GetShape(moving) << "]! This is yet not implemented.\n"
                << "Shape has to be [NxMx1]";

  m_FixedImage = fixed;
  m_MovingImage = moving;

  // if masks were already set, check if geometries fit
  if (m_UseMasksForRegistration)
  {
    if (!(mitk::Equal(*m_FixedImage->GetGeometry(), *m_FixedMask->GetGeometry()) &&
          mitk::Equal(*m_MovingImage->GetGeometry(), *m_MovingMask->GetGeometry())))
    {
      mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(m_FixedImage) << "] and fixed mask image ["
                  << m2::ElxUtil::GetShape(m_FixedMask) << "].\n"
                  << "Moving image [" << m2::ElxUtil::GetShape(m_MovingImage) << "] and moving mask image ["
                  << m2::ElxUtil::GetShape(m_MovingMask) << "].\n"
                  << "Image geometries of mask image and image have to be equal!";
    }
  }
}

void m2::ElxRegistrationHelper::SetDirectory(const std::string &dir)
{
  m_ExternalWorkingDirectory = dir;
}

void m2::ElxRegistrationHelper::SetRegistrationParameters(const std::vector<std::string> &params)
{
  m_RegistrationParameters = params;
}

void m2::ElxRegistrationHelper::SetAdditionalBinarySearchPath(const std::string &path)
{
  m_BinarySearchPath = ElxUtil::JoinPath({path});
}

void m2::ElxRegistrationHelper::CreateWorkingDirectory() const
{
  // Create a temporary directory if workdir not defined

  if (m_ExternalWorkingDirectory.empty())
  {
    m_WorkingDirectory = ElxUtil::JoinPath({mitk::IOUtil::CreateTemporaryDirectory()});
    MITK_INFO << "Create Working Directory: " << m_WorkingDirectory;
  }
  else if (!itksys::SystemTools::PathExists(m_ExternalWorkingDirectory))
  {
    m_WorkingDirectory = ElxUtil::JoinPath({m_ExternalWorkingDirectory});
    itksys::SystemTools::MakeDirectory(m_WorkingDirectory);
    MITK_INFO << "Use External Working Directory: " << m_WorkingDirectory;
  }
}

void m2::ElxRegistrationHelper::GetRegistration()
{
  if (m_FixedImage.IsNull() || m_MovingImage.IsNull())
  {
    MITK_ERROR << "No image set for registration!";
    return;
  }

  const auto exeElastix = m2::ElxUtil::Executable("elastix", m_BinarySearchPath);
  if (exeElastix.empty())
    mitkThrow() << "Elastix executable not found!";

  CreateWorkingDirectory();
  

  if (m_RegistrationParameters.empty())
    m_RegistrationParameters.push_back(m2::Elx::Rigid());

  // Write parameter files
  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto targetParamterFilePath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "pp" + std::to_string(i) + ".txt"});
    const auto element = m_RegistrationParameters[i];
    std::string parameterText;
    if (itksys::SystemTools::FileExists(element) && !itksys::SystemTools::FileIsDirectory(element))
    {
      const auto extension = itksys::SystemTools::GetFilenameExtension(element);
      // TODO: additional checks if p is a real path to an elastix parameter file
      auto ifs = std::ifstream(element);
      parameterText = std::string(std::istreambuf_iterator<char>{ifs}, {});
    }
    else
    {
      parameterText = element;
    }

    // add PointsEuclidianDistance metric
    if (m_UsePointsForRegistration)
    {
      if (parameterText.find("MultiResolution") != std::string::npos)
        ElxUtil::ReplaceParameter(parameterText, "Registration", "\"MultiMetricMultiResolutionRegistration\"");
      else
        ElxUtil::ReplaceParameter(parameterText, "Registration", "\"MultiMetricRegistration\"");
      ElxUtil::ReplaceParameter(
        parameterText, "Metric", "\"AdvancedMattesMutualInformation\" \"CorrespondingPointsEuclideanDistanceMetric\"");
    }
    // Write the parameter file to working directory
    std::ofstream outStream(targetParamterFilePath);
    outStream << parameterText;
    outStream.close();
    m_StatusFunction("Parameter file written: " + targetParamterFilePath);
  }

  const auto movingResultPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "result.0.nrrd"});
  const auto movingMaskResultPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "result.nrrd"});

  const auto fixedPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "fixed.nrrd"});
  const auto movingPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "moving.nrrd"});
  mitk::IOUtil::Save(GetSlice2DData(m_MovingImage), movingPath);
  m_StatusFunction("Moving image written: " + movingPath);
  mitk::IOUtil::Save(GetSlice2DData(m_FixedImage), fixedPath);
  m_StatusFunction("Fixed image written: " + fixedPath);


  // START THE REGISTRATION
  Poco::Process::Args args;
  args.insert(args.end(), {"-f", fixedPath});
  args.insert(args.end(), {"-m", movingPath});
  args.insert(args.end(), {"-out", m_WorkingDirectory});
  if (m_UseMasksForRegistration)
  {
    const auto fixedMaskPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "fixedMask.nrrd"});
    const auto movingMaskPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "movingMask.nrrd"});
    mitk::IOUtil::Save(GetSlice2DData(m_MovingMask), movingMaskPath);
    mitk::IOUtil::Save(GetSlice2DData(m_FixedMask), fixedMaskPath);
    args.insert(args.end(), {"-fMask", fixedMaskPath});
    args.insert(args.end(), {"-mMask", movingMaskPath});
  }

  if (m_UsePointsForRegistration)
  {
    const auto fixedPointsPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "fixedPoints.nrrd"});
    const auto movingPointsPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "movingPoints.nrrd"});
    ElxUtil::SavePointSet(m_MovingPoints, movingPointsPath);
    ElxUtil::SavePointSet(m_FixedPoints, fixedPointsPath);
    args.insert(args.end(), {"-mp", movingPointsPath});
    args.insert(args.end(), {"-fp", fixedPointsPath});
  }

  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto parameterFile = ElxUtil::JoinPath({m_WorkingDirectory, "/", "pp" + std::to_string(i) + ".txt"});
    args.insert(args.end(), {"-p", parameterFile});
  }

  
  m_StatusFunction("Registration started ...");
  
  Poco::Pipe oPipe;
  Poco::ProcessHandle ph(Poco::Process::launch(exeElastix, args, nullptr, &oPipe, nullptr));
  oPipe.close();
  ph.wait();
  
  m_StatusFunction("Registration finished.");
  

  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto transformationParameterFile =
      ElxUtil::JoinPath({m_WorkingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});
    auto ifs = std::ifstream(transformationParameterFile);
    m_Transformations.emplace_back(std::string{std::istreambuf_iterator<char>{ifs}, {}});
    MITK_INFO << "Read transformation: " << transformationParameterFile;
  }

  m_StatusFunction("Transformation parameters assimilated");
  RemoveWorkingDirectory();
}

void m2::ElxRegistrationHelper::SetStatusCallback(const std::function<void(std::string)> &callback)
{
  m_StatusFunction = callback;
}

std::vector<std::string> m2::ElxRegistrationHelper::GetTransformation() const
{
  return m_Transformations;
}

mitk::Image::Pointer m2::ElxRegistrationHelper::WarpImage(const mitk::Image::Pointer data,
                                                          const std::string &pixelType,
                                                          const unsigned char &interpolationOrder) const
{
  const auto exeTransformix = m2::ElxUtil::Executable("transformix", m_BinarySearchPath);
  if (exeTransformix.empty())
    mitkThrow() << "Transformix executable not found!";

  CreateWorkingDirectory();
  m_StatusFunction("Directory created: " + m_WorkingDirectory);

  if (!CheckDimensions(data))
  {
    MITK_ERROR << "Image [" << m2::ElxUtil::GetShape(data) << "]. This is yet not implemented.\n"
               << "Shape has to be [NxMx1]";
    return nullptr;
  }

  const auto imagePath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "data.nrrd"});
  const auto resultPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "result.nrrd"});

  mitk::IOUtil::Save(GetSlice2DData(data), imagePath);
  m_StatusFunction("Moving image written: " + imagePath);

  auto newSpacingX = m_MovingImage->GetGeometry()->GetSpacing()[0];
  auto newSpacingY = m_MovingImage->GetGeometry()->GetSpacing()[1];
  auto newSpacingZ = m_MovingImage->GetGeometry()->GetSpacing()[2];
  std::setlocale(LC_NUMERIC, "C");

  // write all transformations
  for (unsigned int i = 0; i < m_Transformations.size(); ++i)
  {
    auto transformationPath =
      ElxUtil::JoinPath({m_WorkingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});

    auto T = m_Transformations[i];
    // adapt target image geometry

    auto size = m2::ElxUtil::GetParameterLine(T, "Size");
    std::istringstream issSize(size);
    std::vector<std::string> resultsSize(std::istream_iterator<std::string>{issSize},
                                         std::istream_iterator<std::string>());

    const auto sizeX = std::stoi(resultsSize[1]);
    const auto sizeY = std::stoi(resultsSize[2]);

    const auto spacing = m2::ElxUtil::GetParameterLine(T, "Spacing");
    std::istringstream iss(spacing);
    std::vector<std::string> resultsSpacing(std::istream_iterator<std::string>{iss},
                                            std::istream_iterator<std::string>());
    const auto spacingX = std::stod(resultsSpacing[1]);
    const auto spacingY = std::stod(resultsSpacing[2]);

    const auto newSizeX = (sizeX * spacingX) / newSpacingX;
    const auto newSizeY = (sizeY * spacingY) / newSpacingY;

    std::string newSizeString = std::to_string(newSizeX) + " " + std::to_string(newSizeY);
    std::string newSpacingString = std::to_string(newSpacingX) + " " + std::to_string(newSpacingY);

    if (resultsSize.size() == 4 && resultsSpacing.size() == 4)
    {
      const auto sizeZ = std::stoi(resultsSize[3]);
      const auto spacingZ = std::stod(resultsSpacing[3]);
      const auto newSizeZ = (sizeZ * spacingZ) / newSpacingZ;
      newSizeString += " " + std::to_string(newSizeZ);
      newSpacingString += " " + std::to_string(newSpacingZ);
    }

    ElxUtil::ReplaceParameter(T, "ResultImagePixelType", "\"" + pixelType + "\"");
    ElxUtil::ReplaceParameter(T, "ResampleInterpolator", "\"FinalBSplineInterpolatorFloat\"");
    ElxUtil::ReplaceParameter(T, "FinalBSplineInterpolationOrder", std::to_string(interpolationOrder));
    ElxUtil::ReplaceParameter(T, "Spacing", newSpacingString);
    ElxUtil::ReplaceParameter(T, "Size", newSizeString);
    if (i == 0)
    {
      ElxUtil::ReplaceParameter(T, "InitialTransformParametersFileName", R"("NoInitialTransform")");
    }
    else if (i > 0)
    {
      const auto initialTransform =
        ElxUtil::JoinPath({m_WorkingDirectory, "/", "TransformParameters." + std::to_string(i - 1) + ".txt"});
      ElxUtil::ReplaceParameter(T, "InitialTransformParametersFileName", "\"" + initialTransform + "\"");
    }

    MITK_INFO << "Warped image geometry:\n(size) " + newSizeString + "\n(spacing) " + newSpacingString;
    std::ofstream(transformationPath) << T;
  }

  const auto transformationPath = ElxUtil::JoinPath(
    {m_WorkingDirectory, "/", "TransformParameters." + std::to_string(m_Transformations.size() - 1) + ".txt"});

  Poco::Process::Args args;
  args.insert(args.end(), {"-in", imagePath});
  args.insert(args.end(), {"-tp", transformationPath});
  args.insert(args.end(), {"-out", m_WorkingDirectory});

  MITK_INFO << "Start " << exeTransformix << " ...";
  Poco::Pipe oPipe;
  Poco::ProcessHandle ph(Poco::Process::launch(exeTransformix, args, nullptr, &oPipe, nullptr));
  oPipe.close();
  ph.wait();
  MITK_INFO << exeTransformix << " complete";
  m_StatusFunction("Image warped: " + imagePath);

  auto resultData = mitk::IOUtil::Load(resultPath).front();
  mitk::Image::Pointer result = dynamic_cast<mitk::Image *>(resultData.GetPointer());
  result = GetSlice3DData(result);

  if (result->GetDimensions()[2] == 1)
  {
    MITK_WARN << "Restore slice thickness from input data";
    auto s = result->GetGeometry()->GetSpacing();
    s[2] = data->GetGeometry()->GetSpacing()[2];
    result->GetGeometry()->SetSpacing(s);
  }
  RemoveWorkingDirectory();
  return result;
}

void m2::ElxRegistrationHelper::SetRemoveWorkingDirectory(bool val)
{
  m_RemoveWorkingDirectory = val;
}

void m2::ElxRegistrationHelper::RemoveWorkingDirectory() const
{
  try
  {
    if (m_RemoveWorkingDirectory && itksys::SystemTools::PathExists(m_WorkingDirectory) &&
        itksys::SystemTools::FileIsDirectory(m_WorkingDirectory))
    {
      itksys::SystemTools::RemoveADirectory(m_WorkingDirectory);
    }
  }
  catch (std::exception &e)
  {
    MITK_ERROR << "Cleanup ElxRegistrationHelper fails!\n" << e.what();
  }
}