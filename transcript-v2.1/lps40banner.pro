%!
% Banner prolog for LPS 40
%	$Id: lps40banner.pro,v 1.2 1999-01-22 23:11:25 ghudson Exp $
%
% use: push the following strings onto the stack, then execute do_flagpage
%	then end the pagedict
% note: small text at top of page
% client: user@host
% jfname: filename (basename preferred)
% filespec: filename (full path)
% account: username
% submitdate: date submitted
% printq: queue printed on
/pagedict 130 dict def
pagedict begin
/do_flagpage {
/do_setup {
   /pname 64 string statusdict begin printername end def
   /inch {72 mul} def
   /leftmargin 0.75 inch def
   /rightmargin 7.75 inch def
   /tab1 2.0 inch def
   /bottommargin 0.5 inch def
   /topmargin1 10.0 inch def   
   /topmargin2 10.3 inch def
   /athena_place 0.7 inch def
   /productname_place 0.7 inch def
   /sectionb_place1 5.0 inch def  % normal <4 line filespec
   /sectionb_place2 5.5 inch def  % when filespec is long
   /sectionb_place3 4.5 inch def  % for job flag pages
   %DETERMINE VERTICAL START
   /fix_startypos {
      the_note () eq {/ypos topmargin1 def}  {/ypos topmargin2 def} ifelse
      } def
   /full_line rightmargin leftmargin sub def
   /indentedwidth rightmargin tab1 sub def
   /labelwidth tab1 leftmargin sub def
   /midpagex 4.25 inch def
   /bold /Helvetica-Bold findfont def
   /plain /Helvetica findfont def
   /pfont8 plain 8  scalefont  def
   /pfont10 plain 10 scalefont  def
   /bfont10 bold 10 scalefont  def
   /bfont12 bold 12 scalefont  def
   /bfont18 bold 18 scalefont  def
   /bfont22  bold 22 scalefont  def
   /bfont26 bold 26 scalefont  def
   /bfont30 bold 30 scalefont  def
   /bfont34 bold 34 scalefont  def
   /the_productname statusdict /product get def
   /the_athena_text (MIT Project Athena) def
%   /the_jobnumber_text (JOB ) def
   /the_filelength_text ( bytes) def
%SETUP PROCEDURES
   /center {                                 %expects string
      dup stringwidth pop 2 div midpagex exch
     sub ypos moveto } def             
  /nextline {
    /ypos ypos fontheight 1.3 mul sub def
    } def
  /place_it {nextline center show  } def %expects a string on stack
  /downalittle {/ypos ypos 0.3 inch sub def} def
  /downalittlemore {/ypos ypos 0.6 inch sub def} def
  /downinch {/ypos ypos 1.0 inch sub def} def
  /downhalfinch {/ypos ypos 0.5 inch sub def} def  
fix_startypos     %call proc to determine initial ypos
} def %DOSETUP
/breakup_text {
    /width_to_fit exch def    /text exch def    /print_proc exch def
     /index 0 def    /stringlength text length def
    /do_only1line {text print_proc /number_brokenlines 1 def} def
    /do_a_line {
        text lastprintedchar index lastprintedchar sub getinterval print_proc
        /lastprintedchar index def     /accum_width 0 def
        } def
    /do_many_lines {
       /lastprintedchar 0 def   /accum_width 0 def
       {
         text index 1 getinterval 
         stringwidth pop  accum_width add    /accum_width exch def
         accum_width width_to_fit gt  {do_a_line} if
         /index index 1 add def   index stringlength eq {exit} if
        } loop
        text lastprintedchar stringlength lastprintedchar sub
        getinterval print_proc     % above prints leftover characters
        } def %do_mny_lines
      text stringwidth pop width_to_fit gt {do_many_lines} {do_only1line} ifelse
 } def %BREAKUP_TEXT
/join_2strings {
   /str2 exch def /str1 exch def str1 length str2 length add string
   /newstring exch def str1 newstring copy pop /index str1 length def
   newstring index str2 putinterval newstring 
   } def   
/do_note {
    pfont8 setfont  /fontheight 8 def
   () the_note ne                
      { {leftmargin ypos moveto show nextline} the_note full_line
         breakup_text} if     %print note if there is one!
	downalittle
} def %DO_NOTE !
/do_clientuser {
   bfont34 setfont   /xpos leftmargin def   /fontheight 34 def
   nextline   the_clientuser   center   show 
   } def
%/do_jobnumber {
%   the_jobnumber_text the_jobnumber join_2strings
%     /jobnumber_string   exch def
%  bfont26 setfont /fontheight 26 def 0.6 setgray jobnumber_string
%              place_it nextline
%	0 setgray
%   } def
/do_jfname {
      bfont34 setfont   /fontheight 34 def
   {xpos ypos moveto center show nextline} the_jfname full_line   breakup_text
   } def % DO_JFNAME
/do_sectionlabel {
    leftmargin ypos moveto  label show
    } def
/do_sectionvalue {
    currentpoint pop
%    tab1 gt { (\40 \40 ) show }         % METHOD 2, JUST ADD FEW SPACES
     tab1 gt {nextline tab1 ypos moveto} {tab1 ypos moveto } ifelse
    /where_startx currentpoint pop def
    /space_available rightmargin where_startx sub def
    % put args on stack for breakuptext routine
    {where_startx ypos moveto show nextline} value space_available breakup_text
    } def
/do_sectionitem {
   value () ne {do_sectionlabel do_sectionvalue} if
   } def % Do-oneitem
/do_sectionb {
    % Reset ypos for start of this section
    bfont12 setfont  /fontheight 12 def
    /resetypos_fileflag {
        the_filespec stringwidth pop  indentedwidth  div ceiling
       4 lt {/ypos sectionb_place1 def} {/ypos sectionb_place2 def}   ifelse 
       } def 
    the_filespec () eq {/ypos sectionb_place3 def} {resetypos_fileflag} ifelse
    the_account     (Account:) 
    the_filespec     (File:) 
    1 1 2 {pop  /label exch def /value exch def do_sectionitem } for
    downhalfinch
} def % do-sectionb
/do_sectionc {
    pfont10 setfont    /fontheight 10 def
%    the_date_start     (Started:) 
    pname     (Printer node:)
    the_printq    (Printer queue:) 
    the_date_submit     (Submitted:) 
%    the_priority    (Priority:)
%    the_filelength () eq {/the_filelength_string the_filelength def}
%    {/the_filelength_string  the_filelength the_filelength_text
%       join_2strings def} ifelse
%    the_filelength_string    (Length:)        % ditto
        
    1 1 3 { pop /label exch def /value exch def do_sectionitem} for
} def % Do_sectionc
/do_productname {
   bfont18 setfont   /fontheight 18 def  
   leftmargin productname_place moveto   the_productname show
   } def
/right_just {
   exch stringwidth pop sub   % returns x value
   } def
/do_athena {
   the_athena_text dup   rightmargin right_just athena_place moveto show
   FontDirectory /AthenaFont known
	{ /AthenaFont findfont 150 scalefont setfont
	  rightmargin 150 sub athena_place 50 add moveto (A) show
	} if
   } def
%/the_date_start exch def
/the_printq exch def
/the_date_submit exch def
%/the_priority exch def
/the_account exch def
/the_filespec exch def
/the_jfname exch def
%/the_jobnumber exch def
/the_clientuser exch def
/the_note exch def
   do_setup
   do_productname
   do_athena
   do_note 
   do_clientuser   downalittle
%   do_jobnumber    downalittle
   nextline
   do_jfname
   do_sectionb
   do_sectionc
   showpage
   } def %DO_FLAGPAGE
%
%
%use top paper tray if there is one: colored paper
statusdict /setpapertray known 
{1 statusdict begin setpapertray end} if % top paper tray, if known
