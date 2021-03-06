***************
3dKruskalWallis
***************

.. _3dKruskalWallis:

.. contents:: 
    :depth: 4 

| 

.. code-block:: none

    This program performs nonparametric Kruskal-Wallis test for         
    comparison of multiple treatments.                                
    
    Usage:                                                              
    3dKruskalWallis                                                     
    -levels s                      s = number of treatments             
    -dset 1 filename               data set for treatment #1            
     . . .                           . . .                              
    -dset 1 filename               data set for treatment #1            
     . . .                           . . .                              
    -dset s filename               data set for treatment #s            
     . . .                           . . .                              
    -dset s filename               data set for treatment #s            
                                                                        
    [-workmem mega]                number of megabytes of RAM to use    
                                     for statistical workspace          
    [-voxel num]                   screen output for voxel # num        
    -out prefixnamename            Kruskal-Wallis statistics are written
                                     to file prefixname                 
    
    
    N.B.: For this program, the user must specify 1 and only 1 sub-brick  
          with each -dset command. That is, if an input dataset contains  
          more than 1 sub-brick, a sub-brick selector must be used, e.g.: 
          -dset 2 'fred+orig[3]'                                          
    
    INPUT DATASET NAMES
    -------------------
    This program accepts datasets that are modified on input according to the
    following schemes:
      'r1+orig[3..5]'                                    {sub-brick selector}
      'r1+orig<100..200>'                                {sub-range selector}
      'r1+orig[3..5]<100..200>'                          {both selectors}
      '3dcalc( -a r1+orig -b r2+orig -expr 0.5*(a+b) )'  {calculation}
    For the gruesome details, see the output of 'afni -help'.
    
    ++ Compile date = Jan 29 2018 {AFNI_18.0.11:linux_ubuntu_12_64}
