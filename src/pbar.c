/*****************************************************************************
   Major portions of this software are copyrighted by the Medical College
   of Wisconsin, 1994-2000, and are released under the Gnu General Public
   License, Version 2.  See the file README.Copyright for details.
******************************************************************************/

#include "pbar.h"
#include "xim.h"

static void PBAR_button_EV( Widget w, XtPointer cd, XEvent *ev, Boolean *ctd ) ;
static void PBAR_bigmap_finalize( Widget w, XtPointer cd, MCW_choose_cbs *cbs );

static int      bigmap_num=0 ;    /* 31 Jan 2003 */
static char   **bigmap_name ;
static rgbyte **bigmap ;

/*----------------------------------------------------------------------
   Make a new paned-window color+threshold selection bar:

     parent  = parent Widget
     dc      = pointer to MCW_DC for display info
     npane   = initial number of panes
     pheight = initial height (in pixels) of each pane
     pmin    = min value (bottom of lowest pane)
     pmax    = max value (top of highest pane)
     cbfunc  = function to call when a change is made

                 void cbfunc( MCW_pbar * pbar , XtPointer cbdata , int reason )

     cbdata  = data for this call
     reason  = pbCR_COLOR --> color changed
               pbCR_VALUE --> value changed

  WARNING: this code is a mess!  Especially the parts dealing
           with resizing, where the geometry management of the
           Motif widgets must be allowed for.
------------------------------------------------------------------------*/

MCW_pbar * new_MCW_pbar( Widget parent , MCW_DC * dc ,
                         int npane , int pheight , float pmin , float pmax ,
                         gen_func * cbfunc , XtPointer cbdata )

{
   MCW_pbar * pbar ;
   int i , np , jm , lcol , ic , ph ;
   Widget frm ;

   /* sanity check */

   if( npane   < NPANE_MIN        || npane > NPANE_MAX ||
       pheight < PANE_MIN_HEIGHT  || pmin == pmax         ) return NULL ;

   /* new pbar */

   lcol = dc->ovc->ncol_ov - 1 ;  /* last color available */

   pbar = myXtNew( MCW_pbar ) ;

   pbar->top = XtVaCreateWidget( "pbar" , xmBulletinBoardWidgetClass , parent ,
                                     XmNmarginHeight , 0 ,
                                     XmNmarginWidth , 0 ,
                                     XmNheight , npane*pheight+(npane-1)*PANE_SPACING ,
                                     XmNresizePolicy , XmRESIZE_ANY ,
                                     XmNtraversalOn , False ,
                                     XmNinitialResourcesPersistent , False ,
                                  NULL ) ;

   frm = XtVaCreateManagedWidget( "pbar" , xmFrameWidgetClass , pbar->top ,
                                     XmNshadowType , XmSHADOW_ETCHED_IN ,
                                  NULL ) ;

   pbar->panew = XtVaCreateWidget( "pbar" , xmPanedWindowWidgetClass , frm ,
                                      XmNsashWidth , PANE_WIDTH-2*PANE_SPACING,
                                      XmNsashIndent , PANE_SPACING ,
                                      XmNsashHeight , (npane<NPANE_NOSASH) ? SASH_HYES
                                                                           : SASH_HNO ,
                                      XmNmarginHeight , 0 ,
                                      XmNmarginWidth , 0 ,
                                      XmNspacing , PANE_SPACING ,
                                      XmNx , 0 , XmNy , 0 ,
                                      XmNtraversalOn, False ,
                                      XmNinitialResourcesPersistent , False ,
                              NULL ) ;

   if( check_pixmap == XmUNSPECIFIED_PIXMAP )
      check_pixmap = XCreatePixmapFromBitmapData(
                        XtDisplay(parent) , RootWindowOfScreen(XtScreen(parent)) ,
                        check_bits , check_width , check_height ,
#if 0
                        1,0,
#else
                        dc->ovc->pixov_brightest , dc->ovc->pixov_darkest ,
#endif
                        DefaultDepthOfScreen(XtScreen(parent)) ) ;

   /** make the panes **/

   pbar->pane_hsum[0] = 0 ;  /* Dec 1997 */

   for( i=0 ; i < NPANE_MAX ; i++ ){
      ph = (i<npane) ? pheight : PANE_MIN_HEIGHT ;  /* Dec 1997 */
      pbar->pane_hsum[i+1] = pbar->pane_hsum[i] + ph ;

      pbar->panes[i] = XtVaCreateWidget(
                          "pbar" , xmDrawnButtonWidgetClass , pbar->panew ,
                              XmNpaneMinimum , PANE_MIN_HEIGHT ,
                              XmNallowResize , True ,
                              XmNheight , ph ,
                              XmNwidth , PANE_WIDTH,
                              XmNborderWidth , 0 ,
                              XmNmarginWidth , 0 ,
                              XmNmarginHeight , 0 ,
                              XmNhighlightThickness , 0 ,
                              XmNpushButtonEnabled , True ,
                              XmNshadowThickness , 1 ,
                              XmNuserData , (XtPointer) pbar ,
                              XmNtraversalOn , False,
                              XmNinitialResourcesPersistent , False ,
                            NULL ) ;

      if( i < npane ) XtManageChild( pbar->panes[i] ) ;

      XtAddCallback( pbar->panes[i] , XmNactivateCallback , PBAR_click_CB , dc ) ;
      XtAddCallback( pbar->panes[i] , XmNresizeCallback , PBAR_resize_CB , pbar ) ;

      pbar->ov_index[i] = ic = MIN( lcol , i+1 ) ;
      MCW_set_widget_bg( pbar->panes[i] , NULL , dc->ovc->pix_ov[ic] ) ;
   }
   XtManageChild( pbar->panew ) ;

   pbar->panes_sum    = pheight * npane ;
   pbar->num_panes    = npane ;
   pbar->panew_height = pbar->panes_sum + (npane-1)*PANE_SPACING ;

   pbar->pb_CB     = cbfunc ;
   pbar->pb_data   = cbdata ;
   pbar->dc        = dc ;
   pbar->renew_all = 0 ;

   /** make the labels **/

   for( i=0 ; i <= NPANE_MAX ; i++ ){
      int yy ;
      char buf[16] ;

      pbar->pval[i] = pmax - i * (pmax-pmin)/npane ;
      PBAR_labelize( pbar->pval[i] , buf ) ;

      if( i < npane ){
         yy = i * (pheight+PANE_SPACING) ;
         if( i > 0 ) yy -= PANE_LOFF ;
      } else {
#if 1
         yy = pbar->panew_height - PANE_LOFF + PANE_SPACING ;
#else
         yy = pbar->panew_height - 2 * PANE_LOFF + PANE_SPACING ;
#endif
      }

      pbar->labels[i] =  XtVaCreateWidget(
                            " XXXXX" , xmLabelWidgetClass , pbar->top ,
                               XmNrecomputeSize , False ,
                               XmNx , PANE_WIDTH+PANE_SPACING+4 ,
                               XmNy , yy ,
                               XmNborderWidth , 0 ,
                               XmNmarginWidth , 0 ,
                               XmNmarginHeight , 0 ,
                               XmNalignment , XmALIGNMENT_BEGINNING ,
                               XmNhighlightThickness , 0 ,
                               XmNshadowThickness , 0 ,
                             NULL ) ;

      if( KEEP_LABEL(i,npane) ){
         XtManageChild( pbar->labels[i] ) ;
         MCW_set_widget_label( pbar->labels[i] , buf ) ;
      }
   }
   /*-- add _save & mode stuff --*/

   for( np=NPANE_MIN ; np <= NPANE_MAX ; np++ ){
      for( i=0 ; i <= np ; i++ )
         for( jm=0 ; jm < PANE_MAXMODE ; jm++ )
            pbar->pval_save[np][i][jm] = pmax - i * (pmax-pmin)/np ;

      for( i=0 ; i < np ; i++ )
         for( jm=0 ; jm < PANE_MAXMODE ; jm++ )
            pbar->ovin_save[np][i][jm] = MIN(lcol,i+1) ;
   }
   pbar->update_me    = 0 ;
   pbar->mode         = 0 ;
   pbar->hide_changes = 0 ;
   pbar->keep_pval    = 0 ;  /* Dec 1997 */

   for( jm=0 ; jm < PANE_MAXMODE ; jm++ )
      pbar->npan_save[jm] = pbar->num_panes ;

   /*-- 31 Jan 2003: create palettes to choose between for "big" mode --*/

   PBAR_add_bigmap(NULL,NULL) ;

   /*-- 30 Jan 2003: setup the "big" mode for 128 colors --*/

   pbar->bigmode      = 0 ;
   pbar->bigset       = 0 ;
   pbar->bigmap_index = 0 ;
   pbar->bigbot  = -1.0 ; pbar->bigtop = 1.0 ;
   pbar->bigxim  = NULL ;
   for( i=0 ; i < NPANE_BIG ; i++ )
     pbar->bigcolor[i] = bigmap[0][i] ;

   XtAddCallback( pbar->panes[0], XmNexposeCallback, PBAR_bigexpose_CB, pbar ) ;

   XtInsertEventHandler( pbar->panes[0] ,
                         ButtonPressMask ,      /* get button presses */
                         FALSE ,                /* nonmaskable events? */
                         PBAR_button_EV ,       /* event handler */
                         (XtPointer) pbar ,     /* client data */
                         XtListTail ) ;         /* last in queue */

   /*-- go home --*/

   XtManageChild( pbar->top ) ;
   return pbar ;
}

/*-----------------------------------------------------------------------*/
/*! Add a color map for "big" mode.
-------------------------------------------------------------------------*/

void PBAR_add_bigmap( char *name , rgbyte *cmap )
{
   int ii , nn , kk ;

   /* if needed, setup initial color field tables */

#define NBIGMAP_INIT 4
   if( bigmap_num == 0 ){
     bigmap_num     = NBIGMAP_INIT ;
     bigmap_name    = (char **) malloc(sizeof(char *)*NBIGMAP_INIT) ;
     bigmap_name[0] = strdup("Spectrum:red_to_blue") ;
     bigmap_name[1] = strdup("Spectrum:yellow_to_red") ;
     bigmap_name[2] = strdup("Color_circle_AJJ") ;
     bigmap_name[3] = strdup("Color_circle_ZSS") ;
     bigmap         = (rgbyte **) malloc(sizeof(rgbyte *)*NBIGMAP_INIT) ;
     bigmap[0]      = (rgbyte *) malloc(sizeof(rgbyte)*NPANE_BIG) ;
     bigmap[1]      = (rgbyte *) malloc(sizeof(rgbyte)*NPANE_BIG) ;
     bigmap[2]      = (rgbyte *) malloc(sizeof(rgbyte)*NPANE_BIG) ;
     bigmap[3]      = (rgbyte *) malloc(sizeof(rgbyte)*NPANE_BIG) ;
     for( ii=0 ; ii < NPANE_BIG ; ii++ ){
       bigmap[0][ii] = DC_spectrum_AJJ(      ii*(250.0/(NPANE_BIG-1))-5.0,0.8);
       bigmap[1][ii] = DC_spectrum_AJJ( 60.0-ii*( 60.0/(NPANE_BIG-1))    ,0.7);
       bigmap[2][ii] = DC_spectrum_AJJ(      ii*(360.0/(NPANE_BIG-1))    ,0.8);
       bigmap[3][ii] = DC_spectrum_ZSS(360.0-ii*(360.0/(NPANE_BIG-1))    ,1.0);
     }
   }

   if( name == NULL || *name == '\0' || cmap == NULL ) return ;

   nn = bigmap_num ; kk = nn+1 ;

   bigmap_num      = kk ;
   bigmap_name     = (char **) realloc(bigmap_name,sizeof(char *)*kk);
   bigmap          = (rgbyte **) realloc(bigmap,sizeof(rgbyte *)*kk);
   bigmap_name[nn] = strdup(name) ;
   bigmap[nn]      = (rgbyte *) malloc(sizeof(rgbyte)*NPANE_BIG) ;

   for( ii=0 ; ii < NPANE_BIG ; ii++ ) bigmap[nn][ii] = cmap[ii] ;

   POPDOWN_strlist_chooser ; return ;
}

/*-----------------------------------------------------------------------*/

void PBAR_read_bigmap( char *fname , MCW_DC *dc )
{
#define NSBUF 128
  int ii , neq=0 , jj ;
  char name[NSBUF], lhs[NSBUF],rhs[NSBUF], line[2*NSBUF] , *cpt ;
  float  val[NPANE_BIG] , fr,fg,fb , top,bot,del,vv ;
  rgbyte col[NPANE_BIG] , map[NPANE_BIG] ;
  FILE *fp ;

  if( fname == NULL || *fname == '\0' || dc == NULL ) return ;
  fp = fopen(fname,"r"); if( fp == NULL ) return;

  /* get name */

  do{
    cpt = fgets( line , 2*NSBUF , fp ) ;
    if( cpt == NULL ){ fclose(fp); return; }
    name[0] = '\0' ;
    sscanf(line,"%127s",name) ;
  } while( name[0]=='\0' || name[0]=='!' || (name[0]=='/' && name[1]=='/') ) ;

  /* get lines of form "value = colordef" */

  while( neq < NPANE_BIG ){
    cpt = fgets( line , 2*NSBUF , fp ) ;
    if( cpt == NULL ) break ;              /* exit loop */
    lhs[0] = rhs[0] = '\0' ;
    sscanf(line,"%127s = %127s",lhs,rhs) ;
    if( lhs[0] == '\0' || rhs[0] == '\0' ) continue ;
    if( lhs[0] == '!'  || (lhs[0]=='/' && lhs[1]=='/') ) continue ;
    val[neq] = strtod(lhs,&cpt) ;
    if( val[neq] == 0.0 && *cpt != '\0' ){
      fprintf(stderr,"** %s: %s is a bad number\n",fname,lhs); continue;
    }
    ii = DC_parse_color( dc , rhs , &fr,&fg,&fb ) ;
    if( ii ){
      fprintf(stderr,"** %s: %s is bad colorname\n",fname,rhs); continue;
    }
    col[neq].r = (byte)(255.0*fr+0.5) ;
    col[neq].g = (byte)(255.0*fg+0.5) ;
    col[neq].b = (byte)(255.0*fb+0.5) ; neq++ ;
  }
  fclose(fp) ; if( neq < 2 ) return ;

  /* bubble sort val,col pairs */

  do{
   for( jj=ii=0 ; ii < neq-1 ; ii++ ){
    if( val[ii+1] > val[ii] ){
      fr     = val[ii] ; val[ii] = val[ii+1] ; val[ii+1] = fr     ;
      map[0] = col[ii] ; col[ii] = col[ii+1] ; col[ii+1] = map[0] ;
      jj = 1 ;
    }
   }
  } while(jj) ;

  top = val[0] ; bot = val[neq-1] ; if( bot >= top ) return ;
  del = (top-bot)/(NPANE_BIG-1) ;

  for( jj=ii=0 ; ii < NPANE_BIG ; ii++ ){
    vv = top - ii*del ;
    for( ; jj < neq-1 ; jj++ )
      if( vv <= val[jj] && vv >= val[jj+1] ) break ;
    if( vv >= val[jj] ){
      map[ii] = col[jj] ;
    } else if( vv <= val[jj+1] ){
      map[ii] = col[jj+1] ;
    } else {
      fr = (vv-val[jj+1])/(val[jj]-val[jj+1]) ;
      fg = 1.0-fr ;
      map[ii].r = (byte)(fr*col[jj].r + fg*col[jj+1].r + 0.5) ;
      map[ii].g = (byte)(fr*col[jj].g + fg*col[jj+1].g + 0.5) ;
      map[ii].b = (byte)(fr*col[jj].b + fg*col[jj+1].b + 0.5) ;
    }
  }

  PBAR_add_bigmap( name, map ) ; return ;
}

/*-----------------------------------------------------------------------*/
/*! Button 3 event handler for pane #0 of a pbar, used only when
    in "big" mode, to select a color map.
-------------------------------------------------------------------------*/

static void PBAR_button_EV( Widget w, XtPointer cd, XEvent *ev, Boolean *ctd )
{
   MCW_pbar *pbar = (MCW_pbar *) cd ;
   XButtonEvent *bev = (XButtonEvent *) ev ;

#if 0
   if( bev->button == Button2 )
     XUngrabPointer( bev->display , CurrentTime ) ;
#endif

   if( pbar == NULL || !pbar->bigmode ) return ;

   switch( bev->button ){
     case Button3:
       MCW_choose_strlist( w , "Choose Color Field" ,
                           bigmap_num ,
                           pbar->bigmap_index ,
                           bigmap_name ,
                           PBAR_bigmap_finalize , cd ) ;
     break ;

     case Button2:{
       int hh , ii ;
       MCW_widget_geom( pbar->panes[0] , NULL,&hh , NULL,NULL ) ;
       ii = (int)( ((NPANE_BIG-1.0)*bev->y)/(hh-1) + 0.5 ) ;
       fprintf(stderr,"Color[%03d]: R=%03d G=%03d B=%03d #%02x%02x%02x\n",
               ii , (int)pbar->bigcolor[ii].r          ,
                    (int)pbar->bigcolor[ii].g          ,
                    (int)pbar->bigcolor[ii].b          ,
                    (unsigned int)pbar->bigcolor[ii].r ,
                    (unsigned int)pbar->bigcolor[ii].g ,
                    (unsigned int)pbar->bigcolor[ii].b  ) ;
     }
     break ;
   }
   return ;
}

/*--------------------------------------------------------------------*/

static void PBAR_bigmap_finalize( Widget w, XtPointer cd, MCW_choose_cbs *cbs )
{
   MCW_pbar *pbar = (MCW_pbar *) cd ;
   int ii , ind=cbs->ival ;

   if( ind < 0 || ind >= bigmap_num || !pbar->bigmode ){
     XBell( pbar->dc->display,100); POPDOWN_strlist_chooser; return;
   }

   pbar->bigmap_index = ind ;
   for( ii=0 ; ii < NPANE_BIG ; ii++ )
     pbar->bigcolor[ii] = bigmap[ind][ii] ;

   MCW_kill_XImage(pbar->bigxim) ; pbar->bigxim = NULL ;
   PBAR_bigexpose_CB(NULL,pbar,NULL) ;
   if( pbar->pb_CB != NULL ) pbar->pb_CB( pbar, pbar->pb_data, pbCR_COLOR );
   return ;
}

/*--------------------------------------------------------------------*/
/*! Actually redisplay pane #0 in "big" mode.
----------------------------------------------------------------------*/

void PBAR_bigexpose_CB( Widget w , XtPointer cd , XtPointer cb )
{
   MCW_pbar *pbar = (MCW_pbar *) cd ;

   if( pbar == NULL || !pbar->bigmode ) return ;

   /* make an image of what we want to see */

   if( pbar->bigxim == NULL ){
     int ww,hh , ii ;
     MRI_IMAGE *cim ;
     XImage    *xim ;
     byte      *car ;

     MCW_widget_geom( pbar->panes[0] , &ww,&hh , NULL,NULL ) ;
     cim = mri_new( 1,NPANE_BIG , MRI_rgb ) ;
     car = MRI_RGB_PTR(cim) ;
     for( ii=0 ; ii < NPANE_BIG ; ii++ ){
       car[3*ii  ] = pbar->bigcolor[ii].r ;
       car[3*ii+1] = pbar->bigcolor[ii].g ;
       car[3*ii+2] = pbar->bigcolor[ii].b ;
     }
     xim = mri_to_XImage( pbar->dc , cim ) ;
     pbar->bigxim = resize_XImage( pbar->dc , xim , ww,hh ) ;
     MCW_kill_XImage(xim) ; mri_free(cim) ;
   }

   /* actually show the image to the window pane */

   XPutImage( pbar->dc->display , XtWindow(pbar->panes[0]) ,
              pbar->dc->origGC , pbar->bigxim , 0,0,0,0 ,
              pbar->bigxim->width , pbar->bigxim->height ) ;
}

/*--------------------------------------------------------------------*/
/*! Set "big" mode in the pbar -- 30 Jan 2003 - RWCox.
----------------------------------------------------------------------*/

void PBAR_set_bigmode( MCW_pbar *pbar, int bmode, float bot,float top )
{
   if( bmode && bot < top ){ pbar->bigbot = bot; pbar->bigtop = top; }
   pbar->bigmode   = bmode ;
   pbar->update_me = 1 ;
   update_MCW_pbar( pbar ) ;
}

/*--------------------------------------------------------------------*/

static void PBAR_show_bigmode( MCW_pbar *pbar )  /* 30 Jan 2003 */
{
   int ii , yy ;
   char buf[16] ;

   if( pbar == NULL || !pbar->bigmode ) return ;

   if( !pbar->bigset ){   /* set up big mode */

     if( pbar->hide_changes ) XtUnmapWidget( pbar->top ) ;

     /* turn off all but 1 pane and all but 2 labels */

     XtManageChild( pbar->labels[0] ) ;
     XtManageChild( pbar->labels[1] ) ;
     for( ii=2 ; ii <= NPANE_MAX ; ii++ )
       XtUnmanageChild( pbar->labels[ii] ) ;
     XtManageChild( pbar->panes[0] ) ;
     for( ii=1 ; ii < NPANE_MAX ; ii++ )
       XtUnmanageChild( pbar->panes[ii] ) ;
     XtVaSetValues( pbar->panes[0] , XmNheight,pbar->panew_height , NULL ) ;
     XtVaSetValues( pbar->panew    , XmNheight,pbar->panew_height , NULL ) ;
     XtVaSetValues( pbar->top      , XmNheight,pbar->panew_height , NULL ) ;

     if( pbar->hide_changes ) XtMapWidget( pbar->top ) ;

     MCW_widget_geom( pbar->panes[0] , NULL,NULL,NULL , &yy ) ;
     XtVaSetValues( pbar->labels[0] , XmNy , yy , NULL ) ;
     PBAR_labelize( pbar->bigtop , buf ) ;
     MCW_set_widget_label( pbar->labels[0] , buf ) ;

     yy = pbar->panew_height - PANE_LOFF + PANE_SPACING ;
     XtVaSetValues( pbar->labels[1] , XmNy , yy , NULL ) ;
     PBAR_labelize( pbar->bigbot , buf ) ;
     MCW_set_widget_label( pbar->labels[1] , buf ) ;

     pbar->bigset = 1 ;
   }

   /* show the thing */

   PBAR_bigexpose_CB( NULL , pbar , NULL ) ;
}

/*--------------------------------------------------------------------
   make a label for the edge out of the floating value
----------------------------------------------------------------------*/

void PBAR_labelize( float val , char * buf )
{
   float aval = fabs(val) ;
   char prefix[4] ;

   if( val == 0.0  ){ strcpy(buf," 0") ; return ; }

   if( val > 0.0 ) strcpy(prefix," ") ;
   else            strcpy(prefix,"-") ;

        if( aval <= 9.994 ) sprintf(buf,"%s%4.2f",prefix,aval) ;
   else if( aval <= 99.94 ) sprintf(buf,"%s%4.1f",prefix,aval) ;
   else                     sprintf(buf,"%s%4f"  ,prefix,aval) ;
   return ;
}

/*--------------------------------------------------------------------
  pbar pane was clicked --> set its color
----------------------------------------------------------------------*/

void PBAR_click_CB( Widget w , XtPointer cd , XtPointer cb )
{
   MCW_DC * dc = (MCW_DC *) cd ;
   MCW_pbar * pbar = NULL ;
   int ip ;

   XtVaGetValues( w , XmNuserData , &pbar , NULL ) ;
   if( pbar == NULL ) return ;

   if( pbar->bigmode ){   /* 30 Jan 2003: reverse color spectrum */
     rgbyte tc ;
     for( ip=0 ; ip < NPANE_BIG/2 ; ip++ ){
       tc = pbar->bigcolor[ip] ;
       pbar->bigcolor[ip] = pbar->bigcolor[NPANE_BIG-1-ip] ;
       pbar->bigcolor[NPANE_BIG-1-ip] = tc ;
     }
     MCW_kill_XImage(pbar->bigxim) ; pbar->bigxim = NULL ;
     PBAR_bigexpose_CB( NULL , pbar , NULL ) ;
     if( pbar->pb_CB != NULL ) pbar->pb_CB( pbar, pbar->pb_data, pbCR_COLOR );
     return ;
   }

   for( ip=0 ; ip < pbar->num_panes ; ip++ ) if( pbar->panes[ip] == w ) break ;
   if( ip == pbar->num_panes ) return ;

   MCW_choose_ovcolor( w , dc , pbar->ov_index[ip] , PBAR_set_CB , dc ) ;
}

/*--------------------------------------------------------------------*/

void PBAR_set_panecolor( MCW_pbar *pbar , int ip , int ovc ) /* 17 Jan 2003 */
{
   if( pbar == NULL || pbar->bigmode ) return ;  /* 30 Jan 2003 */
   if( ovc > 0 ){
      XtVaSetValues( pbar->panes[ip] ,
                        XmNbackgroundPixmap , XmUNSPECIFIED_PIXMAP ,
                     NULL ) ;
      MCW_set_widget_bg( pbar->panes[ip] , NULL , pbar->dc->ovc->pix_ov[ovc] ) ;
   } else {
      XtVaSetValues( pbar->panes[ip] ,
                        XmNbackgroundPixmap , check_pixmap ,
                     NULL ) ;
   }
}

/*--------------------------------------------------------------------
  actual place where color of pane is changed, and user is callbacked
----------------------------------------------------------------------*/

void PBAR_set_CB( Widget w , XtPointer cd , MCW_choose_cbs * cbs )
{
   MCW_DC * dc = (MCW_DC *) cd ;
   MCW_pbar * pbar = NULL ;
   int ip , jm ;

   if( cbs->ival > 0 && cbs->ival < dc->ovc->ncol_ov ){
      XtVaSetValues( w , XmNbackgroundPixmap , XmUNSPECIFIED_PIXMAP , NULL ) ;
      MCW_set_widget_bg( w , NULL , dc->ovc->pix_ov[cbs->ival] ) ;
   } else {
      XtVaSetValues( w , XmNbackgroundPixmap , check_pixmap , NULL ) ;
   }

   XtVaGetValues( w , XmNuserData , &pbar , NULL ) ;
   if( pbar == NULL ) return ;
   if( pbar->bigmode ) return ;  /* 30 Jan 2003 */

   for( ip=0 ; ip < pbar->num_panes ; ip++ ) if( pbar->panes[ip] == w ) break ;
   if( ip == pbar->num_panes ) return ;

   jm = pbar->mode ;
   pbar->ovin_save[pbar->num_panes][ip][jm] =
                         pbar->ov_index[ip] = cbs->ival ;

   if( pbar->pb_CB != NULL ) pbar->pb_CB( pbar , pbar->pb_data , pbCR_COLOR ) ;
   return ;
}

/*--------------------------------------------------------------------------
   Rotate the colors in a pbar by n locations (+ or -) -- 30 Mar 2001
----------------------------------------------------------------------------*/

void rotate_MCW_pbar( MCW_pbar * pbar , int n )
{
   int ip , iov[NPANE_MAX] , np , kov , jm ;
   Widget w ;
   MCW_DC * dc ;

ENTRY("rotate_MCW_pbar") ;

   if( pbar == NULL || n == 0 ) EXRETURN ;

   if( pbar->bigmode ){             /* 30 Jan 2003: rotate the spectrum */
     rgbyte oldcolor[NPANE_BIG] ;

     MCW_kill_XImage(pbar->bigxim) ; pbar->bigxim = NULL ;
     memcpy(oldcolor,pbar->bigcolor,sizeof(rgbyte)*NPANE_BIG) ;

     while( n < 0 ) n += NPANE_BIG ;  /* make n positive */
     for( ip=0 ; ip < NPANE_BIG ; ip++ )
       pbar->bigcolor[ip] = oldcolor[(ip+n)%NPANE_BIG] ;

     PBAR_bigexpose_CB( NULL , pbar , NULL ) ;

   } else {                         /* the older way */
     dc = pbar->dc ;
     np = pbar->num_panes ;
     jm = pbar->mode ;
     while( n < 0 ) n += np ;  /* make n positive */
     for( ip=0 ; ip < np ; ip++ ) iov[ip] = pbar->ov_index[ip] ;

     for( ip=0 ; ip < np ; ip++ ){
        kov = iov[ (ip+n)%np ] ;  /* new overlay index for ip-th pane */
        w   = pbar->panes[ip] ;
        if( kov > 0 && kov < dc->ovc->ncol_ov ){
           XtVaSetValues( w , XmNbackgroundPixmap , XmUNSPECIFIED_PIXMAP , NULL ) ;
           MCW_set_widget_bg( w , NULL , dc->ovc->pix_ov[kov] ) ;
        } else {
           XtVaSetValues( w , XmNbackgroundPixmap , check_pixmap , NULL ) ;
        }
        pbar->ovin_save[pbar->num_panes][ip][jm] =
                              pbar->ov_index[ip] = kov ;
     }
   }

   if( pbar->pb_CB != NULL ) pbar->pb_CB( pbar , pbar->pb_data , pbCR_COLOR ) ;

   EXRETURN ;
}

/*--------------------------------------------------------------------
  callback when a pane is resized:
    - if the panes don't all add up to the right height, then
      this isn't the last callback in the sequence, and we should
      wait for that one to occur
-----------------------------------------------------------------------*/

void PBAR_resize_CB( Widget w , XtPointer cd , XtPointer cb )
{
   MCW_pbar * pbar = (MCW_pbar *) cd ;
   int i , sum , hh[NPANE_MAX] , yy , ip=-1 , jm ;
   char buf[16] ;
   float pmin , pmax , val ;
   int alter_all = pbar->renew_all ;

   if( pbar == NULL || pbar->renew_all < 0 ) return ;  /* skip it */
   if( pbar->bigmode ) return ;  /* 30 Jan 2003 */

   jm  = pbar->mode ;
   sum = 0 ;
   for( i=0 ; i < pbar->num_panes ; i++ ){
     MCW_widget_geom( pbar->panes[i] , NULL , &(hh[i]) , NULL,NULL ) ;
#ifdef PBAR_DEBUG
printf("resize: read pane # %d height=%d\n",i,hh[i]) ; fflush(stdout) ;
#endif
     sum += hh[i] ;
     if( w == pbar->panes[i] ) ip = i ;
   }

   if( sum != pbar->panes_sum ){
      if( ip != pbar->num_panes - 1 ) return ;
      pbar->panes_sum = sum ;
      MCW_widget_geom( pbar->panew , NULL,&(pbar->panew_height),NULL,NULL) ;
#if 0
      XtVaSetValues( pbar->top , XmNheight , pbar->panew_height , NULL ) ;
#endif
      alter_all = 1 ;
   }

   sum  = 0 ;
   pmax = pbar->pval[0] ;
   pmin = pbar->pval[pbar->num_panes] ;

   for( i=0 ; i <= pbar->num_panes ; i++ ){

#if 0  /* the pre Dec 1997 way */
      val = pmax - sum * (pmax-pmin) / pbar->panes_sum ;
      if( alter_all || val != pbar->pval[i] ){
#else
      if( alter_all || (i>0 && pbar->pane_hsum[i] != sum) ){
#endif

         if( ! pbar->keep_pval ){  /* Dec 1997 */
            val = pmax - sum * (pmax-pmin) / pbar->panes_sum ;
            pbar->pval_save[pbar->num_panes][i][jm] =         /* reset this */
                                      pbar->pval[i] = val ;   /* threshold  */
                                                              /* to match pane size */
         }

         if( KEEP_LABEL(i,pbar->num_panes) ){
            if( i < pbar->num_panes ){
               MCW_widget_geom( pbar->panes[i] , NULL,NULL,NULL , &yy ) ;
               if( i > 0 ) yy -= PANE_LOFF ;
            } else {
#if 1
               yy = pbar->panew_height - PANE_LOFF + PANE_SPACING ;
#else
               yy = pbar->panew_height - 2 * PANE_LOFF + PANE_SPACING ;
#endif
            }

            XtVaSetValues( pbar->labels[i] , XmNy , yy , NULL ) ;
            PBAR_labelize( pbar->pval[i] , buf ) ;
            MCW_set_widget_label( pbar->labels[i] , buf ) ;
         }

      }
      if( i < pbar->num_panes ) sum += hh[i] ;
   }

   pbar->pane_hsum[0] = 0 ;
   for( i=0 ; i < pbar->num_panes ; i++ )
      pbar->pane_hsum[i+1] = pbar->pane_hsum[i] + hh[i] ;

   if( pbar->pb_CB != NULL )
      pbar->pb_CB( pbar , pbar->pb_data , pbCR_VALUE ) ;

   pbar->renew_all = 0 ;
}

/*-------------------------------------------------------------------------
  user want to programatically alter the pbar:
    number of panes, and/or new array of values
---------------------------------------------------------------------------*/

void update_MCW_pbar( MCW_pbar * pbar )
{
   if( pbar == NULL ) return ;
   if( pbar->update_me ){
     if( pbar->bigmode ) PBAR_show_bigmode( pbar ) ;         /* 30 Jan 2003 */
     else                alter_MCW_pbar( pbar , 0 , NULL ) ;
   }
   pbar->update_me = 0 ;
}

void alter_MCW_pbar( MCW_pbar * pbar , int new_npane , float * new_pval )
{
   int i , npane , npane_old , sum , hh , ovc , jm ;
   float pmin , pmax , pval[NPANE_MAX+1] , fhh , rhh ;
   int was_bigset ;

   /* sanity check */

   if( pbar == NULL || new_npane > NPANE_MAX ||
       ( new_npane < NPANE_MIN && new_npane != 0 ) ) return ;

   if( pbar->bigmode ) return ;   /* 30 Jan 2003 */
   was_bigset   = pbar->bigset ;
   pbar->bigset = 0 ;

   /* count of panes, old and new */

   jm              = pbar->mode ;
   npane           = (new_npane > 0) ? new_npane : pbar->num_panes ;
   npane_old       = pbar->num_panes ;
   pbar->num_panes = pbar->npan_save[jm] = npane ;

   if( was_bigset ) npane_old = 1 ;

   /*-- get new value array --*/

   if( new_pval == NULL ){
     for( i=0 ; i <= npane ; i++ ) pval[i] = pbar->pval_save[npane][i][jm] ;
   } else {
     for( i=0 ; i <= npane ; i++ ) pval[i] = new_pval[i] ;
   }
   pmax = pval[0] ;
   pmin = pval[npane] ;

   /*--- make new panes or destroy old ones ---*/

   if( pbar->hide_changes ) XtUnmapWidget( pbar->top ) ;

   /* set new pane colors */

   for( i=0 ; i < npane ; i++ ){
      ovc = pbar->ov_index[i] = pbar->ovin_save[npane][i][jm] ;

      if( ovc > 0 ){
         XtVaSetValues( pbar->panes[i] ,
                           XmNbackgroundPixmap , XmUNSPECIFIED_PIXMAP ,
                        NULL ) ;
         MCW_set_widget_bg( pbar->panes[i] , NULL , pbar->dc->ovc->pix_ov[ovc] ) ;
      } else {
         XtVaSetValues( pbar->panes[i] ,
                           XmNbackgroundPixmap , check_pixmap ,
                        NULL ) ;
      }
   }

#ifdef PBAR_DEBUG
printf("\n"); fflush(stdout) ;
#endif

   pbar->renew_all = -1 ;  /* skip updates for the moment */
   for( i=0 ; i < NPANE_MAX ; i++ )
      XtVaSetValues( pbar->panes[i] , XmNheight , PANE_MIN_HEIGHT , NULL ) ;

   for( i=0 ; i <= NPANE_MAX ; i++ )
      if( KEEP_LABEL(i,npane) ) XtManageChild  ( pbar->labels[i] ) ;
      else                      XtUnmanageChild( pbar->labels[i] ) ;

   if( npane > npane_old ){
      for( i=npane_old ; i < npane ; i++ ){
#ifdef PBAR_DEBUG
printf("manage pane %d\n",i) ; fflush(stdout) ;
#endif

         XtManageChild( pbar->panes[i] ) ;

      }
   } else if( npane < npane_old ){
      for( i=npane_old-1 ; i >= npane ; i-- ){
#ifdef PBAR_DEBUG
printf("unmanage pane %d\n",i) ; fflush(stdout) ;
#endif
         XtUnmanageChild( pbar->panes[i] ) ;
      }
   }

   /* set new pane heights */

   pbar->panes_sum = pbar->panew_height - (npane-1)*PANE_SPACING ;
   for( i=0 ; i <= npane ; i++ ) pbar->pval[i] = pval[i] ;

   sum = pbar->panes_sum ;
   rhh = 0.0 ;
   for( i=0 ; i < npane-1 ; i++ ){
      fhh  = pbar->panes_sum * (pval[i]-pval[i+1]) / (pmax-pmin) ;
      hh   = (int) (rhh+fhh+0.45) ;
      rhh  = fhh - hh ;
      sum -= hh ;
#ifdef PBAR_DEBUG
printf("set pane %d to height %d (top=%g bot=%g float=%g rem=%g sum=%d)\n",
       i,hh,pval[i],pval[i+1],fhh,rhh,sum) ; fflush(stdout) ;
#endif
      XtVaSetValues( pbar->panes[i] , XmNheight , hh , NULL ) ;
   }
#ifdef PBAR_DEBUG
printf("set pane %d to height %d\n",npane-1,sum) ; fflush(stdout) ;
#endif
   XtVaSetValues( pbar->panes[npane-1] , XmNheight , sum , NULL ) ;

   XtVaSetValues( pbar->panew ,
                     XmNheight , pbar->panew_height ,
                     XmNsashHeight , (npane<NPANE_NOSASH) ? SASH_HYES
                                                          : SASH_HNO ,
                  NULL ) ;

   XtVaSetValues( pbar->top , XmNheight , pbar->panew_height , NULL ) ;

   if( pbar->hide_changes ) XtMapWidget( pbar->top ) ;

   pbar->renew_all = 1 ;
   pbar->keep_pval = 1 ;  /* Dec 1997 */
   PBAR_resize_CB( pbar->panes[pbar->num_panes-1] , (XtPointer) pbar , NULL ) ;

   if( pbar->keep_pval ){                  /* Dec 1997 */
      for( i=0 ; i <= npane ; i++ )
         pbar->pval_save[pbar->num_panes][i][jm] =
                                   pbar->pval[i] = pval[i] ;
   }
   pbar->keep_pval = 0 ;

#ifdef PBAR_DEBUG
 { int hh,ww,xx,yy , i ;

   XmUpdateDisplay(pbar->top) ;

   MCW_widget_geom(pbar->top , &ww,&hh,&xx,&yy ) ;
   printf("pbar->top  :  w=%d h=%d x=%d y=%d\n",ww,hh,xx,yy) ; fflush(stdout) ;

   MCW_widget_geom(pbar->panew , &ww,&hh,&xx,&yy ) ;
   printf("pbar->panew: w=%d h=%d x=%d y=%d\n",ww,hh,xx,yy) ; fflush(stdout) ;

   for( i=0 ; i < pbar->num_panes ; i++ ){
      MCW_widget_geom(pbar->panes[i] , &ww,&hh,&xx,&yy ) ;
      printf("pane # %d: w=%d h=%d x=%d y=%d\n",i,ww,hh,xx,yy) ; fflush(stdout) ;
   }
 }
#endif

}

/*-------------------------------------------------------------------------
   Make an image of the pbar (sans handles)
   -- RWCox - 15 Jun 2000
---------------------------------------------------------------------------*/

MRI_IMAGE * MCW_pbar_to_mri( MCW_pbar * pbar , int nx , int ny )
{
   MRI_IMAGE * im ;
   int   ii,npix,kk,ll,jj , sum,hh ;
   float pmin,pmax , rhh,fhh , hfrac ;
   byte rr,gg,bb , *bar ;

   /* check for decent inputs */

   if( pbar == NULL ) return NULL ;
   if( nx < 1 ) nx = 1 ;

   if( pbar->bigmode ){    /* 30 Jan 2003: save spectrum */
     XImage *xim ;
     if( pbar->bigxim == NULL ){
       PBAR_bigexpose_CB(NULL,pbar,NULL) ;
       if( pbar->bigxim == NULL ) return NULL ;
     }
     if( ny < NPANE_BIG ) ny = NPANE_BIG ;
     xim = resize_XImage( pbar->dc , pbar->bigxim , nx,ny ) ;
     im  = XImage_to_mri( pbar->dc , xim , X2M_USE_CMAP|X2M_FORCE_RGB ) ;
     MCW_kill_XImage( xim ) ;
     return im ;
   }

   /** the old way: make the image by brute force **/

   if( ny < 4*pbar->num_panes ) ny = 4*pbar->num_panes ;

   im  = mri_new( nx , ny , MRI_rgb ) ;
   bar = MRI_RGB_PTR(im) ;

   pmax = pbar->pval[0] ;
   pmin = pbar->pval[pbar->num_panes] ;

   hfrac = ny / (pmax-pmin) ;
   rhh  = 0.0 ;
   sum  = ny ;

   /* do each pane */

   for( kk=0 ; kk < pbar->num_panes-1 ; kk++ ){
      fhh  = hfrac * (pbar->pval[kk]-pbar->pval[kk+1]) ; /* wannabe height */
      hh   = (int) (rhh+fhh+0.45) ;                      /* actual height */
      rhh  = fhh - hh ;                                  /* remainder */
      sum -= hh ;                                        /* # pixels left */

      if( pbar->ov_index[kk] > 0 ){                      /* solid color */
         rr = DCOV_REDBYTE  (pbar->dc,pbar->ov_index[kk]) ;
         gg = DCOV_GREENBYTE(pbar->dc,pbar->ov_index[kk]) ;
         bb = DCOV_BLUEBYTE (pbar->dc,pbar->ov_index[kk]) ;

         npix = hh*nx ;
         for( ii=0 ; ii < npix ; ii++ ){
           *bar++ = rr ; *bar++ = gg ; *bar++ = bb ;
         }
      } else {                                           /* check pattern */
         byte bwj , bwi ;
         bwj = 255 ;
         for( jj=0 ; jj < hh ; jj++ ){
            bwi = bwj ;
            for( ii=0 ; ii < nx ; ii++ ){
              *bar++ = bwi ; *bar++ = bwi ; *bar++ = bwi ; bwi = ~bwi ;
            }
            bwj = ~bwj ;
         }
      }
   }

   /* last pane */

   kk = pbar->num_panes-1 ;

   if( pbar->ov_index[kk] > 0 ){                      /* solid color */
      rr = DCOV_REDBYTE  (pbar->dc,pbar->ov_index[kk]) ;
      gg = DCOV_GREENBYTE(pbar->dc,pbar->ov_index[kk]) ;
      bb = DCOV_BLUEBYTE (pbar->dc,pbar->ov_index[kk]) ;

      npix = sum*nx ;
      for( ii=0 ; ii < npix ; ii++ ){
        *bar++ = rr ; *bar++ = gg ; *bar++ = bb ;
      }
   } else {                                           /* check pattern */
      byte bwj , bwi ;
      bwj = 255 ;
      for( jj=0 ; jj < hh ; jj++ ){
         bwi = bwj ;
         for( ii=0 ; ii < nx ; ii++ ){
           *bar++ = bwi ; *bar++ = bwi ; *bar++ = bwi ; bwi = ~bwi ;
         }
         bwj = ~bwj ;
      }
   }

   return im ;
}
