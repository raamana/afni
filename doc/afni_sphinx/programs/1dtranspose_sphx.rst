***********
1dtranspose
***********

.. _1dtranspose:

.. contents:: 
    :depth: 4 

| 

.. code-block:: none

    Usage: 1dtranspose infile outfile
    where infile is an AFNI *.1D file (ASCII list of numbers arranged
    in columns); outfile will be a similar file, but transposed.
    You can use a column subvector selector list on infile, as in
      1dtranspose 'fred.1D[0,3,7]' ethel.1D
    
    * This program may produce files with lines longer than a
       text editor can handle.
    * If 'outfile' is '-' (or missing entirely), output goes to stdout.
    
    ++ Compile date = Jan 29 2018 {AFNI_18.0.11:linux_ubuntu_12_64}
