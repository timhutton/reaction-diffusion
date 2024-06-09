## Ready ##

Ready is a program for exploring [continuous](http://www.wolframscience.com/nksonline/section-4.8) and discrete cellular automata, including [reaction-diffusion](http://mrob.com/pub/comp/xmorphia/) systems and PDEs, on grids and arbitrary meshes. [OpenCL](http://en.wikipedia.org/wiki/OpenCL) is used as the computation engine, to take advantage of the [many-core architectures](http://herbsutter.com/welcome-to-the-jungle/) on graphics cards and modern CPUs. OpenCL also allows rules to be written in a text format and compiled on the fly. Ready supports a compact [XML](http://en.wikipedia.org/wiki/XML)-based file format so that images and rules can be shared easily.

Ready supports 1D, 2D and 3D data, as well as polygonal and polyhedral meshes.

### Download Ready 0.11: ###

<!-- To make the icon for this section, see resources/download_icons.html -->
[![image](https://user-images.githubusercontent.com/647092/106070536-8edff880-60fc-11eb-8574-52d7977b7f9e.png)
](https://github.com/GollyGang/ready/releases/download/0.11.0/Ready-0.11.0-Windows-64bit.zip)
[![image](https://user-images.githubusercontent.com/647092/106070568-9bfce780-60fc-11eb-84f5-9699b44b3a65.png)](https://github.com/GollyGang/ready/releases/download/0.11.0/Ready-0.11.4-Mac.dmg)

<b>Linux:</b>
```
sudo apt-get install libgtk-3-dev libxt-dev ocl-icd-opencl-dev libglu1-mesa-dev libvtk9-dev libwxgtk3.2-dev cmake-curses-gui build-essential git
git clone https://github.com/GollyGang/ready.git
cd ready
cmake -DCMAKE_BUILD_TYPE=Release .
make
```
Full build instructions in [BUILD.txt](https://github.com/GollyGang/ready/blob/gh-pages/BUILD.txt#L95-L122) (please ask if you want binaries)

### About Ready: ###

[Changes](https://GollyGang.github.io/ready/Help/changes.html), [To-do list](https://GollyGang.github.io/ready/TODO.txt), [Credits](https://GollyGang.github.io/ready/Help/credits.html)

Build status on the main branch: &nbsp;&nbsp;&nbsp;
[![Windows](https://github.com/GollyGang/ready/workflows/Windows/badge.svg?branch=gh-pages)](https://github.com/GollyGang/ready/actions?query=workflow%3AWindows+branch%3Agh-pages) &nbsp; [![macOS](https://github.com/GollyGang/ready/workflows/macOS/badge.svg?branch=gh-pages)](https://github.com/GollyGang/ready/actions?query=workflow%3AmacOS+branch%3Agh-pages) &nbsp; [![Ubuntu](https://github.com/GollyGang/ready/workflows/Ubuntu/badge.svg?branch=gh-pages)](https://github.com/GollyGang/ready/actions?query=workflow%3AUbuntu+branch%3Agh-pages)

For questions, [join our mailing list](https://groups.google.com/forum/#!forum/reaction-diffusion). Or you can email [tim.hutton@gmail.com](mailto:tim.hutton@gmail.com).

Screenshots:

<img src="https://user-images.githubusercontent.com/647092/69561331-1ab03f00-0fa5-11ea-9b8e-d669a6211633.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561501-64008e80-0fa5-11ea-9181-3b34bff17bd9.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561550-7975b880-0fa5-11ea-82e2-8a9da40d1606.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561588-8e524c00-0fa5-11ea-979d-8a4c221badb0.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561625-a2964900-0fa5-11ea-987b-90a2bf2fc907.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561697-bb066380-0fa5-11ea-957a-567808338c44.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561739-cd809d00-0fa5-11ea-9ff5-8c0c9aa32fcc.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561773-df624000-0fa5-11ea-84d2-4d816739d071.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561881-0e78b180-0fa6-11ea-9a14-0e1705e454ff.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561922-1f292780-0fa6-11ea-913c-445897712915.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69561972-30723400-0fa6-11ea-8ba3-058085fba7f3.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69562023-43850400-0fa6-11ea-8c66-b5cea5bcc1ac.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69562068-55ff3d80-0fa6-11ea-8a7e-79d86ef80abf.png" height="100px"></img>
<img src="https://user-images.githubusercontent.com/647092/69562095-66171d00-0fa6-11ea-966f-730fb5126bc2.png" height="100px"></img>

Press coverage:
  * New Scientist, August 2012: "[First gliders navigate ever-changing Penrose universe](http://www.newscientist.com/article/dn22134-first-gliders-navigate-everchanging-penrose-universe.html)"

[Citations:](https://scholar.google.com/scholar?oi=bibs&hl=en&cites=17960091338602715353)

  * Mikhail Kryuchkov, Oleksii Bilousov, Jannis Lehmann, Manfred Fiebig & Vladimir L. Katanaev ["Reverse and forward engineering of Drosophila corneal nanocoatings"](https://www.nature.com/articles/s41586-020-2707-9) Nature, volume 585, pages 383–389, 2020.
  * Thies H. Büscher, Mikhail Kryuchkov, Vladimir L. Katanaev and Stanislav N. Gorb ["Versatility of Turing patterns potentiates rapid evolution in tarsal attachment microstructures of stick and leaf insects (Phasmatodea)"](https://doi.org/10.1098/rsif.2018.0281) J. R. Soc. Interface 15, 2018.
  * David H. Ackley , Elena S. Ackley. ["The ulam Programming Language for Artificial Life"](http://www.mitpressjournals.org/doi/pdf/10.1162/ARTL_a_00212) Artificial Life. Nov 2016, Vol. 22, No. 4: 431-450.
  * Kawamata I., Yoshizawa S., Takabatake F., Sugawara K., Murata S. ["Discrete DNA Reaction-Diffusion Model for Implementing Simple Cellular Automaton"](https://link.springer.com/chapter/10.1007/978-3-319-41312-9_14) In: Amos M., CONDON A. (eds) Unconventional Computation and Natural Computation. UCNC 2016. Lecture Notes in Computer Science, vol 9726. Springer, Cham.
  * Shigeki Akiyama and Katsunobu Imai, ["The corona limit of Penrose tilings is a regular decagon"](http://math.tsukuba.ac.jp/~akiyama/papers/automata2016.pdf), Lecture Note in Computer Science 9664, M. Cook and T. Neary (Eds.): AUTOMATA 2016, pp. 35–48, 2016. DOI: 10.1007/978-3-319-39300-1
  * John G. Maisey and John S. S. Denton. ["Dermal Denticle Patterning in the Cretaceous Hybodont Shark Tribodus limae (Euselachii, Hybodontiformes), and Its Implications for the Evolution of Patterning in the Chondrichthyan Dermal Skeleton"](http://www.bioone.org/doi/full/10.1080/02724634.2016.1179200) Journal of Vertebrate Paleontology 36(5):e1179200, 2016 doi: http://dx.doi.org/10.1080/02724634.2016.1179200
  * Elmeligy Abdelhamid SH, Kuhlman CJ, Marathe MV, Mortveit HS, Ravi SS ["GDSCalc: A Web-Based Application for Evaluating Discrete Graph Dynamical Systems"](https://doi.org/10.1371/journal.pone.0133660) PLoS ONE 10(8):e0133660, 2015. https://doi.org/10.1371/journal.pone.0133660
  * Nathanael Aubert and Olaf Witkowski. ["Reaction-Diffusion Risk: Chemical Signaling in a Conquest Game"](https://www.cs.york.ac.uk/nature/ecal2015/late-breaking/164.pdf) Late Breaking Proceedings of the European Conference on Artificial Life 2015, pp. 29-30, 2015.
  * Kaier Wang, Moira L. Steyn-Ross, D. Alistair Steyn-Ross, Marcus T. Wilson, Jamie W. Sleigh, Yoichi Shiraishi. ["Simulations of pattern dynamics for reaction-diffusion systems via SIMULINK"](http://www.biomedcentral.com/1752-0509/8/45) BMC Systems Biology 8(14), 2014.
  * Aubert, Nathanaël, Clément Mosca, Teruo Fujii, Masami Hagiya, and Yannick Rondelez. ["Computer-assisted design for scaling up systems based on DNA reaction networks"](http://www.yannick-rondelez.com/wp-content/uploads/2014/03/Untitled-37758-1.pdf) Journal of The Royal Society Interface 11, no. 93, 2014.
  * Millán, Emmanuel N., Paula Martínez, Graciela Verónica Gil Costa, María Fabiana Piccoli, Alicia Marcela Printista, Carlos Bederian, Carlos García Garino, and Eduardo M. Bringa. ["Parallel implementation of a cellular automata in a hybrid CPU/GPU environment"](http://sedici.unlp.edu.ar/bitstream/handle/10915/31730/Documento_completo.pdf?sequence=1) In XVIII Congreso Argentino de Ciencias de la Computación. 2013.
  * Adam P. Goucher. ["Gliders in cellular automata on Penrose tilings"](http://cp4space.files.wordpress.com/2012/11/2012-penrose-gliders.pdf), Journal of Cellular Automata, Volume 7, Number 5-6, p. 385-392, 2012.

Please cite Ready as:

    Tim Hutton, Robert Munafo, Andrew Trevorrow, Tom Rokicki, Dan Wills.
    "Ready, a cross-platform implementation of various reaction-diffusion systems."
    https://github.com/GollyGang/ready

Blog coverage:
  * The Christev Creative, April 2015: "[Negative Worms in Reaction Diffusion](https://christevcreative.com/2015/04/17/negative-worms-in-reaction-diffusion/)"
  * Tim's blog, December 2012: "[Ready 0.5](https://timhutton.github.io/2012/12/20/36454.html)"
  * GPU Science, September 2012: "[Reaction-Diffusion by the Gray-Scott Model with OpenCL](http://gpuscience.com/physicalscience/reaction-diffusion-by-the-gray-scott-model-with-opencl/)"
  * Complex Projective 4-Space, August 2012: "[Are you Ready?](https://cp4space.wordpress.com/2012/08/24/are-you-ready/)"
  * Mikael Hvidtfeldt Christensen, August 2012: "[Reaction-Diffusion Systems](http://blog.hvidtfeldts.net/index.php/2012/08/reaction-diffusion-systems/)"
  * Hacker News, July 2012: "[First glider discovered in a cellular automata on an aperiodic tiling](http://news.ycombinator.com/item?id=4298515)"
  * Aperiodical.com, July 2012: "[Ready: reaction-diffusion simulator](http://aperiodical.com/2012/07/ready-reaction-diffusion-simulator/)" [[2](http://aperiodical.com/2012/08/a-glider-on-an-aperiodic-cellular-automaton-exists/)]
  * Tim's blog, September 2010: "[reaction-diffusion](http://ferkeltongs.livejournal.com/32025.html)"

Other packages:
  * [TexRD](http://www.texrd.com/) (closed source)
  * [Visions of Chaos](http://softology.com.au/voc.htm) (closed source)
  * [CAPOW](https://github.com/rudyrucker/capow)

Wanted rules: (help needed)
  * L.Decker 2003, derived from Maginu's (from TexRD)
  * L.Decker 2002, derived from Ginzburg-Landau (from TexRD)
  * L.Decker 1998, derived from Brusselator (from TexRD)

Videos:

Full playlist here: https://www.youtube.com/playlist?list=PL35FD96A5F8236109

<!-- You can use e.g. http://embedyoutube.org/ to make these thumbnail links -->
[![](http://img.youtube.com/vi/1XbV0_UviQI/0.jpg)](http://www.youtube.com/watch?v=1XbV0_UviQI "Mutually-catalytic spots 2")
[![](http://img.youtube.com/vi/vI85hlsbJJw/0.jpg)](http://www.youtube.com/watch?v=vI85hlsbJJw "Laplacian Growth - twin coral")
[![](http://img.youtube.com/vi/B8jUH-2AP14/0.jpg)](http://www.youtube.com/watch?v=B8jUH-2AP14 "Laplacian Growth")
[![](http://img.youtube.com/vi/KKPFmfWjwt4/0.jpg)](http://www.youtube.com/watch?v=KKPFmfWjwt4 "FitzHugh-Nagumo")
[![](http://img.youtube.com/vi/te_JU3RZ2eM/0.jpg)](http://www.youtube.com/watch?v=te_JU3RZ2eM "Double-slit experiment")
<a href='http://www.youtube.com/watch?feature=player_embedded&v=KJe9H6qS82I' target='_blank'><img src='http://img.youtube.com/vi/KJe9H6qS82I/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=5TubQw4f_RU' target='_blank'><img src='http://img.youtube.com/vi/5TubQw4f_RU/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=3oqap32-Tg0' target='_blank'><img src='http://img.youtube.com/vi/3oqap32-Tg0/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=c9EoI9tw6NE' target='_blank'><img src='http://img.youtube.com/vi/c9EoI9tw6NE/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=tZHOGFA1KZE' target='_blank'><img src='http://img.youtube.com/vi/tZHOGFA1KZE/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=XYyX4GpzhmQ' target='_blank'><img src='http://img.youtube.com/vi/XYyX4GpzhmQ/0.jpg' width='425' height=344 /></a>
