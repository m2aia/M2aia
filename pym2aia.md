ImzML access using M²aia in Python 
----------------------------------

pyM²aia is a Python interface for mass spectrometry imaging with focus on Deep Learning utilizing M²aia's I/O libraries.

[github.com/m2aia/pym2aia](https://github.com/m2aia/pym2aia)

#### Package installation 
Consider to install pyM²aia in an [virtual environment](https://packaging.python.org/en/latest/tutorials/installing-packages/#creating-and-using-virtual-environments).

```sh
python3 -m venv .venv
source .venv/bin/activate

pip install m2aia
```


#### Getting started 
Examples [repository](https://github.com/m2aia/pym2aia-examples):
- [ImzML Meta-data access](https://github.com/m2aia/pym2aia-examples/blob/main/Example_I_ImzMLMetaData.ipynb)
- [Signal processing](https://github.com/m2aia/pym2aia-examples/blob/main/Example_II_SignalProcessing.ipynb)
- [Ion image generation](https://github.com/m2aia/pym2aia-examples/blob/main/Example_III_IonImages.ipynb)
- [Spectral strategy - Peak Learning](https://github.com/m2aia/pym2aia-examples/blob/main/Example_IV_A_AutoEncoder_IndividualModels.ipynb)
- [Spatial strategy - Unsupervised Ion-image clustering](https://github.com/m2aia/pym2aia-examples/blob/main/Example_V_UnsupervisedClustering.ipynb)
- [Spatio-spectral streategy: Autoencoder](https://github.com/m2aia/pym2aia-examples/blob/main/Example_VI_AutoEncoder_SpatioSpectral.ipynb)
- [Spatio-spectral streategy: Pixel-wise classification](https://github.com/m2aia/pym2aia-examples/blob/main/Example_VII_Classification_SpatioSpectral.ipynb)

[Documentation](https://data.jtfc.de/pym2aia/sphinx-build/html/m2aia.html#module-m2aia.ImageIO)

