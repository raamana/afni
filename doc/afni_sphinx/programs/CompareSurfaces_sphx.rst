***************
CompareSurfaces
***************

.. _CompareSurfaces:

.. contents:: 
    :depth: 4 

| 

.. code-block:: none

    
       Usage:    CompareSurfaces 
                 -spec <Spec file>
                 -hemi <L or R>
                 -sv1 <volparentaligned1.BRIK>
                 -sv2 <volparentaligned2.BRIK> 
                 [-prefix <fileprefix>]
    
       NOTE: This program is now superseded by SurfToSurf
    
       This program calculates the distance, at each node in Surface 1 (S1) to Surface 2 (S2)
       The distances are computed along the local surface normal at each node in S1.
       S1 and S2 are the first and second surfaces encountered in the spec file, respectively.
    
       -spec <Spec file>: File containing surface specification. This file is typically 
                          generated by @SUMA_Make_Spec_FS (for FreeSurfer surfaces) or 
                          @SUMA_Make_Spec_SF (for SureFit surfaces).
       -hemi <left or right>: specify the hemisphere being processed 
       -sv1 <volume parent BRIK>:volume parent BRIK for first surface 
       -sv2 <volume parent BRIK>:volume parent BRIK for second surface 
    
    Optional parameters:
       [-prefix <fileprefix>]: Prefix for distance and node color output files.
                               Existing file will not be overwritten.
       [-onenode <index>]: output results for node index only. 
                           This option is for debugging.
       [-noderange <istart> <istop>]: output results from node istart to node istop only. 
                                      This option is for debugging.
       NOTE: -noderange and -onenode are mutually exclusive
       [-nocons]: Skip mesh orientation consistency check.
                  This speeds up the start time so it is useful
                  for debugging runs.
    
       [-novolreg]: Ignore any Rotate, Volreg, Tagalign, 
                    or WarpDrive transformations present in 
                    the Surface Volume.
       [-noxform]: Same as -novolreg
       [-setenv "'ENVname=ENVvalue'"]: Set environment variable ENVname
                    to be ENVvalue. Quotes are necessary.
                 Example: suma -setenv "'SUMA_BackgroundColor = 1 0 1'"
                    See also options -update_env, -environment, etc
                    in the output of 'suma -help'
      Common Debugging Options:
       [-trace]: Turns on In/Out debug and Memory tracing.
                 For speeding up the tracing log, I recommend 
                 you redirect stdout to a file when using this option.
                 For example, if you were running suma you would use:
                 suma -spec lh.spec -sv ... > TraceFile
                 This option replaces the old -iodbg and -memdbg.
       [-TRACE]: Turns on extreme tracing.
       [-nomall]: Turn off memory tracing.
       [-yesmall]: Turn on memory tracing (default).
      NOTE: For programs that output results to stdout
        (that is to your shell/screen), the debugging info
        might get mixed up with your results.
    
    
    Global Options (available to all AFNI/SUMA programs)
      -h: Mini help, at time, same as -help in many cases.
      -help: The entire help output
      -HELP: Extreme help, same as -help in majority of cases.
      -h_view: Open help in text editor. AFNI will try to find a GUI editor
      -hview : on your machine. You can control which it should use by
               setting environment variable AFNI_GUI_EDITOR.
      -h_web: Open help in web browser. AFNI will try to find a browser.
      -hweb : on your machine. You can control which it should use by
              setting environment variable AFNI_GUI_EDITOR. 
      -h_find WORD: Look for lines in this programs's -help output that match
                    (approximately) WORD.
      -h_raw: Help string unedited
      -h_spx: Help string in sphinx loveliness, but do not try to autoformat
      -h_aspx: Help string in sphinx with autoformatting of options, etc.
      -all_opts: Try to identify all options for the program from the
                 output of its -help option. Some options might be missed
                 and others misidentified. Use this output for hints only.
      
    
       For more help: https://afni.nimh.nih.gov/ssc/ziad/SUMA/SUMA_doc.htm
    
    
       If you can't get help here, please get help somewhere.
    
    Compile Date:
       Jan 29 2018
    
    
        Shruti Japee LBC/NIMH/NIH shruti@codon.nih.gov Ziad S. Saad SSSC/NIMH/NIH saadz@mail.nih.gov 
