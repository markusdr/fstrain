fstrain
=======

A toolkit for training finite-state models

Build Instructions
------------------

1. Install Boost (http://www.boost.org/)

2. Install R (http://www.r-project.org/)

3. Install OpenFst (http://www.openfst.org/)

4. Build fstrain:

        mkdir Release
        cd Release
        cmake ../fstrain
        make
        export FSTRAIN_ROOT=$(pwd)
        cd ..
        make -C test

    If your OpenFst is not installed in a standard system directory
    (like /usr/local or similar) then cmake cannot find it. In that
    case, use this cmake command:

        cmake -DOPENFST_ROOT=/my/path/to/openfst ../fstrain
    
    where /my/path/to/openfst contains OpenFst's include/ and lib/
    directories.

    The same works for Boost: Use the BOOST_ROOT variable to specify
    the Boost location if it is not installed in a system directory:

    cmake -DBOOST_ROOT=/my/path/to/boost ../fstrain
    
5. Done!

Markus Dreyer, markus.dreyer@gmail.com
