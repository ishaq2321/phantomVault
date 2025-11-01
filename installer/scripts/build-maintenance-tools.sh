#!/bin/bash

# PhantomVault Maintenance and Diagnostic Tools Builder
# Creates comprehensive maintenance and diagnostic utilities

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[TOOLS]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/installer/build"
TOOLS_DIR="$BUILD_DIR/tools"

print_status "Building maintenance and diagnostic tools..."
print_status "Tools directory: $TOOLS_DIR"

# Clean and create tools directory
rm -rf "$TOOLS_DIR"
mkdir -p "$TOOLS_DIR"/{scripts,diagnostics,maintenance,update}

# Create system diagnostic tool
create_diagnostic_tool() {
    print_status "Creating system diagnostic tool..."
    
    cat > "$TOOLS_DIR/diagnostics/phantomvault-diagnostics.sh" << 'EOF'
#!/bin/bash
# PhantomVault System Diagnostics Tool
echo "PhantomVault System Diagnostics"
echo "Generated: $(date)"
echo "System: $(uname -a)"

# Check installation
if [ -d "/opt/phantomvault" ]; then
    echo "✓ Installation directory exists"
    if [ -f "/opt/phantomvault/bin/phantomvault" ]; then
        echo "✓ Service executable found"
    else
        echo "✗ Service executable not found"
    fi
else
    echo "✗ Installation directory not found"
fi

# Check service status
if systemctl is-active phantomvault &> /dev/null; then
    echo "✓ PhantomVault service is running"
else
    echo "✗ PhantomVault service is not running"
fi

echo "Diagnostic complete"
EOF
    
    chmod +x "$TOOLS_DIR/diagnostics/phantomvault-diagnostics.sh"
    print_success "System diagnostic tool created"
}

# Create maintenance tool
create_maintenance_tool() {
    p
# Check ins
check_installation() {
    print_header "Installation Check"
    
    if  ]; then
L_DIR"
        
        if [ -hen
            print_success "Service executable
            
            # Check executable perssions
          ]; then
               
            else
                print_error "Service executable is not executable"
            fi
            
            #ze
            local size=$(stat -c%s "$INS")
            if [ "$size" -gt 10en
                print_success "Se"
          e
      
            fi
        else
    
   fi
      
        # Check GUI files
en
            print_success "G"
        else
           "
        fi
        
        # Check documentation
       ]; then

        else
             found"
        fi
    else
        print_error "Installation directory not found: $INSTALL_DIR"
    fi
    
    # Check symlink
    if [ -
    "
        
")
        if [ "$target" = "$INSTALL_DIR/bin/phantomv; then
 target"
        else
            print_warning "Symlink points to unexpected target: $target"
        fi
    else
omvault"
    fi}eck service statusck_service() {  print_header "Service Status in "$@"unction
main f
# Run ma"
}
n)e integratioGUI updatpdates.js (ck-uus "  - cheint_stat
    pr)"ementaganupdate m-updater (tomvault phan"  -rint_status "
    pes)ce utiliti(maintenanance intenmalt-mvauphanto  -  "int_status pr"
   cs)diagnostiics (system ostvault-diagn phantomtus "  -nt_sta    prie)"
ed interfacools (unifi-tomvault"  - phants nt_statuprils:"
    too"Available t_status 
    prinfully!"successs created ic tooland diagnostnce  "Maintenaint_success pr  "
    
 -updaterphantomvaultBUILD_DIR/"$er.sh" updatantomvault-te/ph_DIR/upda"$TOOLS -sf 
    lne"aintenancantomvault-mR/phD_DIIL" "$BUtenance.shvault-main/phantomaintenanceLS_DIR/mOO$T   ln -sf "ostics"
 t-diagn/phantomvaulILD_DIR" "$BUgnostics.shmvault-diastics/phantoDIR/diagnoOOLS_-sf "$T"
    ln -toolsaulttomvphan_DIR/UILDsh" "$Btools.omvault-hantR/p$TOOLS_DI "
    ln -sfsy accessnks for eaymlite s# Crea     
scripts
   pper_create_wra    tegration
_in_gui_updatereateol
    ctodate_e_up  creat
  toolntenance_eate_maitool
    crstic__diagno create    
   s..."
ld procestools buiagnostic dind  a maintenanceartings "Statu  print_st) {
  in(
maecution# Main ex
"
}
createdpts  scri wrapperess "Toolt_succrin
    pt-tools.sh"ulIR/phantomva "$TOOLS_D  chmod +x 
  
EOF
   @""$in ma menu
ain

# Run m    done
}e..." -r
nuntinter to cop "Press E      read -"
  "       echo   
 c
             esa;
 ect 0-8." ;se selead option. Plror "Invali) print_er   *
          0 ;;e!"; exitss "Goodby print_succe 0)         r ;;
  kup_manage 8) bac     
      nitor ;;nce_moorma) perf  7        ;;
  og_analyzer   6) l        ;;
  nspector ) vault_i      5       ;;
e_service) manag        4
    n_updater ;;) ru          3nce ;;
  run_maintena       2) cs ;;
     stirun_diagno    1) 
        ine $REPLY         cas    
 ""
          echo  r
" -n 1 - (0-8): elect option"S read -p      ow_menu
     sh   rue; do
  ile t) {
    whmain(loop
n menu Mai
}

# " backupsstemore syest and rreateool will cus "This t_statint   pr
 oming soon"- Cager ackup Manrning "Bnt_wa) {
    primanager(

backup_ources"
} and resrmancesystem perfol monitor ilhis tool w"Ts int_statu
    pring soon" - Comore Moniterformancg "Pinrint_warn
    p) {tor(ance_moni}

performty logs"
d securiystem an analyze sol willis tostatus "Thrint_"
    pg soonCominAnalyzer - ing "Log   print_warnzer() {
  ly

log_ana"
}entsntlt coe vauand validatt will inspec tool his"Tstatus   print_"
   Coming soon Inspector -Vaultg "nt_warnin {
    pri_inspector()vaultl tools
additionafor unctions laceholder f
}

# Pc
    esa;;" ntio"Invalid operror    *) print_;
     pager ;-n 20 --no-antomvault  -u phctlnal jour   5)    ault ;;
 tomvanphtl status temc     4) sysed" ;;
   e restartervic "Sccess& print_suvault &omstart phant rectlo system3) sud  ;
       stopped" ;ss "Service print_succelt && phantomvautemctl stop) sudo sys        2d" ;;
e starteics "Servint_succes&& prlt vauphantomctl start stem sy     1) sudo  Y in
 case $REPL  
    "
   echo "
    1 -r(1-5): " -nlect option "Se read -p    "
echo "
    vice logs") Show ser   echo "5tatus"
 ervice s"4) Show secho "
    eervicart s3) Rest"echo 
    p service"tocho "2) S    eservice"
"1) Start  echo ons:"
   optiervice "Scho ""
    e
    echo      fi
 ing"
  unn is not rervicemVault sg "Phantoarnin   print_w    else
     "
pid"PID: $atus int_st     prvault)
   ue phantomainPID --val -p Mowctl shid=$(system   local png"
      runni isservicelt "PhantomVauess succprint_       
 ; thenv/nulldeult &> /antomvative ph is-acf systemctls
    ie statuk servic    # Chec   
ger"
 Service Manaer "  print_headvice() {
  ernage_sger
mae mana

# Servic
    fi
}nd"outool not fate Upd"nt_error        pri   else
      esac
 " ;;
   alid option "Invrort_er prin      *);
      " version ;r.shpdateantomvault-uIR/update/phRIPT_D 3) "$SC          stall ;;
 er.sh" inupdatlt-omvauupdate/phantIR/RIPT_D2) "$SC           ;;
  check dater.sh"ault-upmvanto/phR/updateCRIPT_DI  1) "$S           in
e $REPLY
        cas"
        ho " ec    -r
     1(1-3): " -nect option d -p "Sel       rea"
    echo "      version"
current "3) Show  echo
        sudo)" (requiresestall updat) Insecho "2
        s"ate for updhecko "1) C     ech  ns:"
 iooptho "Update         ec" ]; then
er.shault-updattomve/phanatupdDIR/CRIPT_-f "$S{
    if [ ter() updapdater
run_Run u
# 
fi
}    ound"
e tool not fncena "Maintror    print_er    se
 elsac
         e
  " ;;id option "Invalerrorprint_    *) 
        ll ;;nce.sh" aintenantomvault-mantenance/phaT_DIR/maiRIP   7) "$SC        
 " report ;;enance.shmvault-maintantophnce/R/maintenaIPT_DI   6) "$SCR  ;;
       h" restart nance.sult-mainteantomvantenance/phPT_DIR/mai"$SCRI       5) 
     ;te ;.sh" updaintenancet-mantomvaulnance/phaDIR/mainte$SCRIPT_   4) "
         " repair ;;enance.shaintt-m/phantomvaulncetena_DIR/main "$SCRIPT    3)        ptimize ;;
nance.sh" oault-maintece/phantomvtenaninCRIPT_DIR/ma  2) "$S
          " clean ;;shmaintenance.t-phantomvaulntenance/DIR/maiPT_  1) "$SCRI         in
 case $REPLY 
        "
        o "    ech-r
     1 (1-7): " -nn ect optio-p "Selad 
        re"ho "        ec"
o)sudquires reenance (all maint "7) Run     echo
    "ate reportener"6) G      echo 
  "uires sudo) (reqicestart servo "5) Res   ech
     "res sudo)equiation (re configurat Upd   echo "4)    ty"
 rit integulvaRepair echo "3) 
        ge"oralt stvau Optimize echo "2)        s"
filey arorempean tho "1) Cl   ec"
     tions:ce opaintenanho "M    ecen
    sh" ]; thnance.ault-maintephantomvenance/intCRIPT_DIR/maf [ -f "$S
    ienance() {
run_maint maintenance# Run fi
}


   ot found"ics tool n"Diagnostint_error 
        prelse    ics.sh"
lt-diagnosthantomvauics/pst_DIR/diagnoCRIPT      "$Shen
  " ]; tstics.shgnoomvault-diacs/phantnostiT_DIR/diag -f "$SCRIPif [() {
    diagnosticsstics
run_Run diagno
# 
 ""
}echo"
    xit"0) E
    echo    echo """
 tem backups sysd restore- Create an     er   agup ManBackho "8)   ecources"
  esmance and rorystem perftor s- Moni Monitor   mance7) Perforho "s"
    ecty logcurim and sesystenalyze  A -     Analyzer    ) Log ho "6s"
    ec contente vaultdatt and vali Inspec    -ctor   nspe Iultcho "5) Va"
    erviceVault set Phantomp, restarto st,tar Sger       - Mana"4) Service    echo  updates"
llinsta and  - Check for      Checker  pdate "3) U"
    echoepair system rize, andimn, opt- Clea Tool      ce) Maintenan"2 echo on"
   ratid configuem health an- Check systnostics     System Diag "1)
    echoo "":"
    echtool to runt a ho "Selec
    ecTools"lt hantomVaueader "Pprint_h {
    nu()
show_menuain me

# Show m" && pwd)"}")H_SOURCE[0]me "${BAScd "$(dirna"$(R=SCRIPT_DI
ectorydirGet script }

# $1"; } NCD}[ERROR]${ -e "${REchoerror() { eprint_"; }
${NC} $1NING]WARLLOW}[ho -e "${YE { ecng()int_warni"; }
pr{NC} $1SUCCESS]$"${GREEN}[{ echo -e ss() _succe; }
print${NC} $1"UE}[TOOLS]ho -e "${BL) { ecint_status(}"; }
pr $1 ===${NC{BLUE}==="\n$ echo -e der() {int_hea0m'

pr3['\03
NC=;34m'33[0'
BLUE='\0'\033[1;33mYELLOW=m'
\033[0;32
GREEN='m'033[0;31='\s
RED Color
#t -e

ses
ol tonostice and diagintenancntomVault maPhaace for all terfUnified inr
# aunche Tools LtomVault

# Phann/bash#!/bi< 'EOF'
 <ols.sh"ault-tophantomvLS_DIR/> "$TOOcat t
    s scrip main tool  # Create"
    
  ..cripts.ol wrapper s tos "Creatingint_statu   prripts() {
 e_wrapper_scts
creatcripapper ste tool wrrea
}

# Cd"ation createpdate integrs "GUI ut_succes    prinEOF
    
cker;
UpdateCheorts = e.exp
}

modul;
    }l)ervas.checkInt, thi    }   pdates();
 heckForUautoChis.      t    => {
  nterval(()         setIally
 periodicCheck     //  
        00);
       }, 300tes();
   datoCheckForUpthis.au          (() => {
    setTimeouts)
      ondsec30 rt (after  sta on app    // Check
    () {rtAutoCheck

    sta  }    }
  nfo);
    alog(updateIdateDi this.showUp       await             
    }
        eturn;
        r    
        );edVersionipp, sk:'onfor versication finotiing pphecker] SkiUpdateCole.log('[        cons       ersion) {
 nfo.latestV=== updateIppedVersion f (ski     i     
  ionsersped vog for skipshow dial Don't     //    
                Version();
pedis.getSkipVersion = thkippedconst s        
    ) { (updateInfo   if    
  e);
       tes(falsrUpdais.checkFoo = await thdateInfst up  con) {
      dates(ForUpautoCheck async     }

   ull;
turn n    re        }

    error);:', sionkipped ver read sailed toteChecker] Fpda'[Usole.error(con          ) {
  erroratch (       } c     }
        n;
dVersiokippeg.s confi   return            8'));
 ath, 'utf(configPreadFileSyncfs..parse(JSON config = onst    c     {
        nfigPath))ync(co.existsSf (fs i           ;
g.json')e-confia'), 'updat('userDatthn(app.getPah.joipatPath = t config        cons {
       try{
     n() edVersioippgetSk  
  
    }

        }rror);sion:', e verave skipped to sFailedcker] heateC.error('[Updole      cons  {
    tch (error) ca } 
        null, 2));ig,ingify(confJSON.strPath, eSync(configeFilwritfs.            on };
ersin: viokippedVersconfig = { s      const ');
      sononfig.jte-c'upda'), serData('uath.getPh.join(app patfigPath =const con              try {
    on) {
  sion(versipedVerveSkip    sa
   }
e;
 ponssult.res   return re   
          }
       break;
             ;
    ersion)nfo.latestVdateIedVersion(upsaveSkipps.       thi        on
  This Versi: // Skip case 2    ak;
           bre           app start
 xt  again on neWill check/        /  r
       Late Remind Me   case 1: //    
       break;         l);
      nloadUrdownfo.dateIExternal(upenell.op       sh       ate
   Updownloadase 0: // D     c      onse) {
 (result.respwitch         s       
 });
        lId: 1
cance     
       aultId: 0,    def      ion'],
   This Vers, 'Skipter'Lamind Me te', 'Redawnload Ups: ['Do   button       
  aseNotes}`,teInfo.releupda notes:\n${Release\ntVersion}\no.currenteInf: ${updaonrent versi: `Cur detail  
         ailable`,sion} is avestVero.latpdateInf ${uPhantomVault: `gemessa     
       vailable',pdate A   title: 'U,
         pe: 'info'     ty
       ssageBox({wMedialog.sho = await st result
        condateInfo) {ialog(updateDc showUp
    asyn0;
    }
  return    
           }
       n 1;
 rt) retur1part > v2paif (v        -1;
    rn tu v2part) re (v1part <   if     
        
        || 0;s[i] art = v2partnst v2p       co     ] || 0;
ts[iart = v1parnst v1p         co+) {
   gth); i+rts.len, v2pas.lengthv1partath.max( i < Mt i = 0;    for (le
         ;
   ap(Number)lit('.').m version2.spt v2parts =    cons    mber);
').map(Nuplit('.version1.s = 1parts    const v{
    , version2) ion1ers(vVersionsompare

    c
    }        });     });
       meout'));
Request tiror('ct(new Er reje         y();
      roequest.dest           r    > {
 , () =10000.setTimeout(     request           
 });
             );
      ject(error      re        
  => {ror) or', (er('erruest.on     req  
                });
    ;
         })           
         }                ror);
  reject(er                     error) {
 tch ( ca     }            
   seInfo);ve(releasol      re           
       rse(data);= JSON.paleaseInfo onst re           c            ry {
         t           () => {
 ('end', .onsponse    re          
              });
                    
nk;+= chu      data          {
     ) => nkata', (chun('dsponse.o   re            
         
        ; '' let data =            {
   nse) => (respo,        }         }
         
   er'cklt-UpdateChehantomVau: 'Pnt'ge     'User-A           rs: {
    ade        he
        teUrl, {his.upda.get(t = httpsonst request          c => {
  e, reject)resolvomise((turn new Pr   ree() {
     atestReleasasync fetchL

    }
    }        turn null;
re                

           }
         ion.');nnectt cour internese check yoleapdates. P for uckailed to che    'F            
    d', heck Faile'Update CrBox(roErdialog.show         
       Dialog) {date (showNoUpif                
);
        tes:', errordag for upor checkinr] ErrkedateChecerror('[Up   console.
         rror) {  } catch (e       
    
       rn null;     retu             
  }
                 });
             sion}`
  Verntthis.curre ${ version:Currentdetail: `         
           e',p to datis ut tomVaul'Phanssage:   me              
    ble',ilapdates Ava Utitle: 'No                  
  e: 'info',         typ          
 ageBox({owMesslog.sh  dia             
 ialog) {wNoUpdateD (shoe if } els         
           };t
       hed_apublisseInfo.: relea publishedAt         
          ml_url,seInfo.htelearl: rnloadU    dow                dy,
seInfo.boleaNotes: reease        rel            
testVersion,sion: la  latestVer               
   rsion,tVeis.currenn: thrrentVersio          cu
          rn {tu       re      le) {
   vailabf (isUpdateA         i  
            sion}`);
 ertVates{l, Latest: $ntVersion}${this.currer] Current: ckeUpdateChee.log(`[ consol     
                  n) < 0;
latestVersioon, entVersi(this.currrsionscompareVeis. thble =ilaAvasUpdate    const i     '');
    (/^v/,me.replaceeInfo.tag_na releasstVersion =  const late   
                       }
  n');
      matioforse inetch relea to filed('FaErrorhrow new        t       fo) {
  Inelease    if (!r      ;
  se()ReleaatestetchLs.f= await thieleaseInfo onst r       c       
         = now;
  s.lastCheck   thi
                              }
k
    checince last/ Too soon surn null; /   ret           
  teDialog) {wNoUpdaval && !shos.checkInterk < thiheclastCow - this.(n         if    ate.now();
 Dnst now =    co        
           
 ..');pdates. for uecking Chr]ateCheckeg('[Updonsole.lo   c     {
           try
  = false) {og NoUpdateDialUpdates(showcheckFor
    async 0;
    }
= Check  this.last  rs
     / 24 hou0 * 1000; / 60 * 6 = 24 *tervalckIn this.che      st';
 eases/lateelntomVault/r1/pha232ishaqrepos/ub.com/pi.githttps://aeUrl = 'hat   this.upd  n();
   p.getVersiorsion = apcurrentVehis.
        t() {constructor
    hecker {dateCUplass 

cth');e('pairrequ= th pa');
const require('fst fs = 
cons'https');quire( rest https =on');
contrre('elec = requi }log, shellapp, dia{ */

const ecking
 update chautomatic on GUI for ectrh Elittegrates w Inr
 * CheckeUI UpdatemVault G*
 * Phanto
/*" << 'EOF'updates.jscheck-_DIR/update/"$TOOLS > UI
    catecker for Gate chreate upd  
    # C
  "tion... integraateI upd GU "Creating_statusrintn() {
    pgratiopdate_integui_ucreate_tegration
pdate inI uCreate GU
# ted"
}
tool creapdate "U cessnt_suc  prir.sh"
  atemvault-updpdate/phantoDIR/uLS_d +x "$TOOhmo  c
    
  $@"
EOF"
main pdater

# Run u
}sac    e    ;;
1
        t        exi  
   w_usage    sho        n: $1"
ptioown oor "Unkn print_err            *)
        ;;
           _usage
       show    lp|-h)
 --he  help|
      ;;            _version"
n: $currentult versiotomVaho "Phan         ec  sion)
 ent_ver=$(get_currrsion_verrent  local cu     on)
     rsi        ve     ;;
         fi
   
       ates"for updt check rror "Cannoprint_e         lse
                e    fi
            
   ile""$update_fe all_updat       inst             
e" ]; thenate_fil[ -n "$upd&& ? -eq 0 ] f [ $   i        
     n")rsioveest_e "$latupdatwnload_te_file=$(do local upda        l
       d instalnload an # Dow        
                       
/^v//') | sed 's"' -f4| cut -d'"[^"]*"'  *name":p -o '"tag_v/null | greRL" 2>/de "$UPDATE_Ul -s$(currsion=test_vel laloca          
      st version # Get late           en
    ; th -eq 2 ]result    elif [ $   e"
      up to datadyss "Alret_succe     prin      
     n theq 0 ];$result -e [          if  
            t=$?
 al resuloc l          ates)
 or_updk_fatus=$(checate_stupd     local ll)
        insta;
               ;   fi
           "
  tall inso $0e, run: sudtall updatus "To insnt_statpri         
       o ""ech             n
   the; 2 ]ult -eq  if [ $res        $?
   cal result=      lo    pdates
   check_for_u       check)
    
        eck}" inchase "${1:-    c {
n()
maiin execution

# Ma""
} echo e"
   lp messagis hew th      Sho  ho "  helpec"
    ent versionrr Show cuversion    ho "  "
    ec sudo)eses (requirtall updatand ins   Download nstall    echo "  ies"
  e updatablr avail  Check fok     checho "  "
    ec "Options: echo"
   "
    echo PTION]"[Oage: $0 "Usho  ec   o ""
l"
    echdate TooomVault Upnt  echo "Phage() {
  w_usahosage
s

# Show u
    fi
}eturn 1  r        
     fi
   
      om backup"tored fr"Rest_success prin           ult
 omvaantctl start ph    system
        STALL_DIR"lt" "$INomvauantCKUP_DIR/phBA -r "$      cp"
      L_DIR$INSTALm -rf "        r
    ; thenvault" ]phantomIR/"$BACKUP_D-d   if [      ackup
 re from bsto Re   #
     
        ."m backup..g froorin "Restnt_status        pri"
er updatet aftto stare failed r "Servicint_erro
        pri
    else   f     R"
DIUP_ $BACKpreserved:ckup Baus "tatrint_s    p       else
   
      removed"p "Backucess print_suc     "
       UP_DIR"$BACK -rf  rm     
      then[Yy]$ ]];  $REPLY =~ ^ [[        ifho

        ec 1 -r" -n/N): ? (yove backup "Rem read -pul
       ccessfsuup if ckup ba    # Clean      
    sion"
   _verion: $newNew vers "t_success       prin)
 version_current_$(getn= new_versio   local
     sionver  # Verify  
             fully"
ssd succelleUpdate instaess "t_succ      prin; then
  v/null/detomvault &> ve phan is-actictlstem sy  ifsleep 2
  ation
    stally in   # Veriflt
    
 omvaustart phant  systemctl ce..."
  lt servig PhantomVaurtinStatus "int_sta
    prcetart servi 
    # Res"
   e_file -f "$updatt"
    rmp_extracrf "$temup
    rm -lean    # C
    
 
    fipdated"ion umentatDocus "nt_succes   priR/"
     LL_DI"$INSTA" scted_dir/docraextp -r "$   c
     L_DIR/docs" "$INSTAL rm -rfhen
       s" ]; tcted_dir/docd "$extra
    if [ -ntationocume  # Update di
    
  "
    fdatedups fileess "GUI _succ print      "
 TALL_DIR/"$INSgui" racted_dir/cp -r "$ext    
    ui"L_DIR/gSTAL-rf "$IN      rm ; then
  r/gui" ]xtracted_di"$e   if [ -d resent
 es if pGUI fil   # Update    
   fi
 pdated"
  able urvice executccess "Sent_su       priomvault"
 /bin/phantSTALL_DIR"$IN chmod +x "
       L_DIR/bin/STALult" "$INin/phantomva/bcted_dir "$extra      cp]; then
  " phantomvaultdir/bin/acted_ -f "$extr if [ files
   new  # Install   
      fi
  tract"
="$temp_exxtracted_dir      e
   ]; thenacted_dir""$extrz 
    if [ -| head -1)ult*" phantomva "*ype d -nameextract" -ttemp_(find "$racted_dir=$ocal extll
    l instad files andacteFind extr    #  fi
    
  n 1
  retur      mat"
 e file foratported updsupor "Unerr      print_else
  act"
    temp_extrle" -d "$ate_fi -q "$updunzip   
     ip ]]; then*.ze" == "$update_fillif [[ ct"
    era$temp_ext"-C _file" updatezf "$r -x  ta     ]]; then
 gz ar.e" == *.t_fil[[ "$updatef e
    ie typon filbased act   # Extr
     act"
 xtrp_ep "$tem   mkdir -"
 xtract-$$mvault-e/phanto="/tmptractcal temp_ex 
    lo..."
   ing update"Installstatus   print_
  l updatestal and in # Extract
        done
 - 1))
   t=$((timeout  timeoup 1
             slee do
  /dev/null; &>vault -f phantom && pgreput -gt 0 ]timeohile [ $10
    wt=oumeocal ti    l stop
tor service t fo # Wai
    
   l || true 2>/dev/nulphantomvaulttop systemctl s  "
  vice...lt serVauomPhant"Stopping status 
    print_top service
    # S    fi
    R"
: $BACKUP_DI created"Backupnt_success      pri"
   BACKUP_DIR/L_DIR" "$AL"$INST   cp -r     en
 ; th" ]IR$INSTALL_D -d "
    if [   IR"
 KUP_Dir -p "$BAC"
    mkdackup..."Creating bs tuint_staup
    pre back# Creat
       fi
     1
     return   "
  sudo)leges (useot privirequires rollation e instar "Updatint_erro    pr
     then0 ];UID" -ne $E    if [ "g as root
k if runnin# Chec
    date"
    g Uptallinader "Ins    print_he
e="$1"l update_filcalo
    date() {all_upnstll update
i# Instale"
}

ownload_fi  echo "$d
  fi
    
    eturn 1   r"
     ableavailol nload to "No dowrornt_er     pri  else
       fi
    urn 1
   ret           oad_dir"
ownl"$d   rm -rf          "
edload failown"Drint_error           p   else
    d"
   ompletenload cs "Dowces   print_suc
         _url"; thenload" "$downad_fileownloget -O "$d    if when
    ull; t> /dev/nt &and -v wgemmcoi
    elif       fn 1
         returdir"
     "$download_    rm -rf        "
 edwnload fail"Dorint_error  p                else
ed"
   mpletoad coDownluccess "     print_shen
       url"; tad_downlo"$file" nload_o "$dow-L -url       if cn
  ev/null; therl &> /d -v cumand if com   
   
 date..."oading upwnl"Doint_status 
    prt-update"phantomvaulownload_dir/"$d_file=cal downloaddate
    lonload up 
    # Dow_dir"
   adnloowkdir -p "$d-$$"
    mpdatelt-uhantomvau"/tmp/pload_dir=down    local tory
direcownload ary deate tempor  
    # Crad_url"
  downlo: $ad URLwnlotus "Dorint_sta    
    p

    esac.zip" ;;ad_url}${downlooad_url=" downl   windows)    g" ;;
 ad_url}.dmdownlo"${nload_url=macos)   dow        
ar.gz" ;;oad_url}.t="${downlload_urlnux)   down  li     in
  tform"la"$pn
    case extensiopropriate d ap   # Ad"
    
 $platformersion-$vhantomvault-on/pad/v$versis/downloREPO/releaseHUB_hub.com/$GIT://githttps"rl=wnload_ual do
    locoad URLct downlru  # Const 
      fi

       return 1"
    (uname -s)tform: $d plapportensuror "Uint_er  pr
      ]; thenown" kn"un= " platform  if [ "$  c
    

    esa" ;;unknownrm="atfo    pl     *)    ;;
   dows" rm="winS*) platfo|MINGW*|MSY   CYGWIN*;;
     ="macos" rm*)  platforwin        Da
linux" ;; platform="   Linux*)  n
     ame -s)" ie "$(un
    casormlatfl p    locarm
ne platfoetermi
    # D
    Update"ding Downloa "nt_header1"
    priversion="$
    local d_update() {
downloapdateoad uownli
}

# D    f
e availablepdatturn 2  # U
        re          fi
   es"
   lease_not$re     echo "
       " notes:us "Release_stat   printn
         tes" ]; theelease_no[ -n "$r       if n/g')
  's/\\n/\' -f4 | sedd'"' | cut -^"]*"ody": *"['"bp -o o" | gret_infcho "$lates=$(eease_notesrel   local     s
 e notet releas     # Ge     
   ion"
   rsest_ve$latersion → rent_v $curvailable: a"Updateus int_stat     pr else
   n 0
       retur"
    p to date is ultPhantomVauuccess "   print_s   hen
  ; t ]t_version"es$latsion" = "ver"$current_f [ ns
    ire versio   # Compan"
    
 est_versio: $latsion"Latest vert_status rin  
    p
  fiturn 1
    re  
      on"st versiparse latet ould nont_error "C    pri
    " ]; thent_version$latesif [ -z "
    ')
    d 's/^v// -f4 | seut -d'"'' | c*"*"[^"]": _nameep -o '"tago" | grt_infho "$lates$(ecersion= latest_vocal    ln
st versioe late# Pars     
   fi
  rn 1
   retu
       on"se informatielea fetch rFailed torror " print_een
        ]; thst_info"$late -z " if [ 
   fi
    1
    turn  re
      ed)"/wget requirurld (cent foun cli"No HTTPror int_er
        prelse   ll)
 /dev/nuTE_URL" 2>UPDAet -qO- "$=$(wgt_infolates
        ll; then&> /dev/nuand -v wget ommelif cull)
    v/n_URL" 2>/deATE"$UPD=$(curl -s latest_info    
    ; then/dev/null>  &d -v curlman com    iftest_info
ocal la    l
   .."
 lease.test relaor g GitHub f "Checkinus  print_stattion
  e informareleasGet latest     # fi
    

    rn 1  retule"
      ilabon avat connectinterne"No i_error   print   l; then
   ul/dev/nb.com &> hug -c 1 git    if ! pinectivity
nternet connCheck i  # "
    
  _version: $currentversionent "Currstatus rint_   prsion)
 current_vet_=$(get_versionurren   local c"
    
  for Updates"Checkinger   print_head() {
  datesor_uptes
check_for updaCheck fi
}

#  fION"
   VERSNT_ "$CURRE echo          else
 n"
 "$versio       echoN")
 NT_VERSIOo "$CURRE|| ech-1 | head .[0-9]\+' ]\+\[0-9\+\. '[0-9]ll | grep -oev/nursion 2>/d --veantomvault"bin/phL_DIR/STALINsion=$("$ver    local     able
om executt version frgeo  t     # Trythen
   ; tomvault" ]hann/pTALL_DIR/bi [ -f "$INS   if() {
 ent_versionget_currrsion
 ve current"

# Check%M%S)-%Hm%ddate +%Y%backup-$(lt-aup/phantomv"/tmR=
BACKUP_DI"aultantomvph/opt/"INSTALL_DIR=st"
leases/latePO/reB_REos/$GITHUhub.com/rep/api.gitps:/="htt
UPDATE_URLt"antomVaulaq2321/phshREPO="i"
GITHUB_="1.0.0RENT_VERSION
CURionnfigurat

# Co $1"; }OR]${NC}"${RED}[ERR{ echo -e rror() 
print_e }1";${NC} $W}[WARNING]LOEL "${Y) { echo -eg(print_warnin
 $1"; }CCESS]${NC}N}[SU -e "${GREE() { echoesst_succ; }
prin} $1"TE]${NCUE}[UPDA "${BLecho -eatus() { rint_st; }
pC}"=${N==1  $${BLUE}===cho -e "\nder() { ehea
print_0m'
NC='\033['
[0;34mUE='\033
BL33[1;33m'ELLOW='\0[0;32m'
YN='\033m'
GREE0;31ED='\033[ors
RCol
# e


set -allationing and instckte chec updatomatil
# Au Update TootomVault Phan

#!/bin/bash< 'EOF'
#dater.sh" <-upomvaultate/phantLS_DIR/updOO$T    cat > " 
..."
   anismch update meeating "Crnt_status  pri {
  tool()ate_update_nism
creechaupdate meate  Cred"
}

#reatnce tool caintenaccess "M_su
    printnance.sh"temainntomvault-hace/panen_DIR/maint"$TOOLS  chmod +x    
  
EOF
 in "$@"ntenance
ma# Run mai

esac
};
     ;        it 1
   ex     
       sageshow_u            "
on: $1ptinown oror "Unkprint_er       *)
     ;;
                   usage
 how_       s|-h)
     elp-hlp|-he      ;;
            port
  ate_re     gener       rvices
start_se   re      ration
   figuon    update_c   s
     repair_vault   
         ltstimize_vau    op       _files
 empan_t       cle   tem"
   "sysgesilecheck_priv               all)
         ;;
 
       ortrate_repene      g    
      report)
      ;;          _services
estart    r        )
  restart    
        ;;      ation
iguronfe_c updat         e)
     updat       ;;
          ults
repair_va          r)
       repai         ;;
  s
    ult optimize_va          
 optimize);;
              s
      an_temp_file   cle      an)
           clelp}" in
${1:-he
    case "{
main() onin executi
# Mao ""
}
    ech"
 messagethis helpw ho      Slp  o "  he   echsudo)"
 res sks (requiance tanten all mai      Runl    alcho "  e"
   porte reintenancrate mane    Ge"  report  ho "
    ecsudo)res s (requiault serviceart PhantomV   Restrt  o "  restach"
    eres sudo)tion (requiconfiguratem  Update sysate     cho "  upd"
    erity vault integ     Repairrepair  "  
    echot storage"ze vaulze    Optimi"  optimi    echo nd logs"
ry files apora  Clean tem  clean        echo "ons:"
 tiOp
    echo " echo """
   [OPTION]"Usage: $0   echo cho ""
  
    eTool"ce anault Mainteno "PhantomV) {
    echage(show_usage
Show us"
}

# _filet "$reportry
    caummaDisplay s
    # "
    ileport_f saved: $re reportintenances "Masucces    print_e"
    
port_fil  } > "$re
             fi
  found"
   lts "  No vau   echo 
         lse
        eotal_files": $tTotal Fileso "          ech  )
  c -lev/null | w-type f 2>/dR/vaults" R_DATA_DIUSEs=$(find "$al_fileal totoc   l            
   nt"
      ou$vault_ctal Vaults: o "  Toch      e
       -l)wc | ev/null2>/dlt_*" ame "vau -npe dvaults" -tyER_DATA_DIR/USind "$(ft=$t_counul   local va         then
 ults" ];A_DIR/vaR_DAT"$USE-d  if [       cs:"
 istiStato "Vault    ech
      statistics   # Vault       
     ""
 o         echi
 f    
    $1}'r Data: " Use{print " awk 'ull | dev/nR" 2>/USER_DATA_DI"$h   du -s       
   henA_DIR" ]; tAT"$USER_Dd if [ -             fi
 1}'
  ion: " $allatInst"  print  | awk '{ll/nu" 2>/devR_DITALLh "$INS -s         du; then
   LL_DIR" ] "$INSTA-d [     if  :"
  sk Usageecho "Di      ge
  Disk usa       # 
       ""
  ho  eci
            fped"
   s: StopStatu "  cho         e  else
   
      "ry} KBemo ${mry:mo"  Me echo            o "0")
/null || echdev2>/" $pidss= -p "ps -o rmory=$( me      local"
      id$p"  PID:       echo   
    ICE_NAME")e "$SERV-valuID - -p MainPl showd=$(systemct  local pi  "
        nningtatus: Ru"  S      echo hen
      l; t&> /dev/nulVICE_NAME" $SERive "s-actsystemctl if         i Status:"
"Service      echo s
  tatue sServic   # 
         "
         echo "   
 -a)"$(unamem: o "Syste       ech"
 ed: $(date)Generato "      ech"
  nce ReportenaainttomVault MPhan  echo "
      {  "
    
  %M%S).log%Y%m%d-%Hce-$(date +aintenanntomvault-m="/tmp/pha_fileort local rep
       Report"
 tenanceer "Mainprint_head) {
    report(erate_ence report
gintenanate maner
}

# Ge    fiE_NAME"
VICl -u $SERalctogs: journeck ltus "Ch print_sta      "
  serviceto startor "Failed t_err prinlse
       ly"
    euccessfulce started sviuccess "Serprint_s        
 thenll;> /dev/nuNAME" &RVICE_SE"$ is-active systemctl if   
 ep 2ed
    slee startervicerify s # V    
   ICE_NAME"
SERVl start "$systemct   e..."
 lt servicntomVauing Pha "Startint_status  prrvice
  t se
    # Star     fi
   
       fiue
 lt || tromvau -f phantll -9       pki
     "rminationing tercully, fogracefot stop ervice did nning "Srint_war      p
      thendev/null; omvault &> /-f phantpgrep    if 
              done
       - 1))
imeout =$((teoutim t           sleep 1
         ull; do
   t &> /dev/n phantomvaul -fepgr && p ] 0out -gt [ $time  while
      out=10l time   loca   ate
  min terfully process to Wait for      #    
  "
     opped"Service st_success        printNAME"
 E_op "$SERVICemctl stsystn
        ll; the/nu" &> /devERVICE_NAME-active "$Smctl is syste   if"
 ervice...ult sg PhantomVatoppin"S_status int    pr
ullygracefservice # Stop 
    
    ystem"vileges "sheck_pri
    c    "
icesmVault Servting Phantor "Restar print_headees() {
   vicstart_ser
reafelyservices sRestart  fi
}

# che"
   font cahed  "Refresnt_success    pri    
 true||null ev/ 2>/dfc-cache -f         then
ll;ev/nu-cache &> /d-v fcif command cache
    efresh font 
    # R fi
    abase"
   datted MIME ccess "Upda print_su       true
 v/null ||mime 2>/deare/ase /usr/shmime-databpdate-    uhen
    ev/null; t /d-database &>e-mimepdatv uf command -  i
  tabasee MIME da
    # Updat    
    fi
"tabase desktop dadatedcess "Up_suc print
       | truell |/nuevs 2>/dation/applicrer/sha/usse -databaesktope-d  updatthen
       /dev/null; ase &>esktop-databupdate-dcommand -v     if base
p datadesktoUpdate #         
"
    fi
figurationemd conoaded systcess "Relt_suc       prinn-reload
 mctl daemo    systehen
    ull; t> /dev/nctl &v system -nd if commaon
   gurati confiystemdoad s Rel
    
    #"emeges "systheck_privil  
    con"
  ratistem Configupdating Sy"Unt_header 
    prion() {figuratie_conation
updatigurm confteysate s
# Upd"
}
suesred ispaied $ree - repairpletk comty checriault integcess "Vucint_s 
    pr  ne
 do   
 
        fi    donei
              f  )
        + 1)aired (repd=$(   repaire              
   le"_fiatametad"$}' > "'"rupted").corata_fileme "$metad(basena": "'"$up, "backd_metadata""corrupteerror":    echo '{"           
      ure JSON struct basicpair re Try to  #            
      "uptedcorradata_file." "$metadata_filep "$met      c            e
  ted filcorrupckup of # Create ba                    ")"
ile$metadata_f(basename "data file: $d metaorruptening "Ct_warinpr           n
         ; the 2>&1llv/nuile" >/de"$metadata_fson.tool python3 -m j    if !             do
_file;  -r metadatae readull | whilv/n" 2>/de*.json"ata" -name dir/metadt_$vaulnd "     fi      n
  ]; the/metadata"_dird "$vault if [ -les
        metadata fieck        # Ch 
one
       
        d   fi         ired + 1))
ed=$((repaepair         r    
    $dir"y:ectorirssing dted mi"Creatus _sta  print              /$dir"
irp "$vault_d   mkdir -             then
 dir" ];r/$$vault_di [ ! -d "    if   do
     rs[@]}"; uired_dieqr in "${rfor di      ackup")
  mp" "bta" "te "metadafolders"_dirs=("redal requi
        locories existdirectrequired Ensure 
        #     
    _id"vaultvault: $hecking "Cs int_statupr     ")
   _dir$vaultname "(based=$ult_iva   local ; do
     ult_dire read -r val | whil 2>/dev/nul*"ame "vault_e d -n -typDIR/vaults"$USER_DATA_nd "
    fit structurepair vaulCheck and re    
    # repaired=0
    local 
    
   fiurn
    ret   air"
  o repund taults fotatus "No vint_s  prhen
      ults" ]; tDIR/va$USER_DATA_! -d "if [ 
    y"
    ult IntegritRepairing Vader "ea  print_h
  lts() {epair_vaugrity
rvault inte
# Repair 
}
   fi
 timized"y opalreadrage is lt stoauess "Vnt_succ
        prilseMB"
    ed_mb}ed ${saveete - savation complrage optimizcess "Stoucrint_s   p)
      1024)24 /ved / 10_satotalmb=$((local saved_     hen
    ]; taved" -gt 0otal_s"$tif [ 
    
      donefi
  
        up")" "$backname $(baseressed:"Comptus && print_stal v/nul" 2>/dekup"$bacgzip              ]]; then
 != *.gz"$backup"& [[  &up" ]-f "$backf [        iup; do
 -r backhile read  wl |/nule +7 2>/dev -mtimackup"ame "*.b -nults"A_DIR/va$USER_DAT"
    find ."
    backups..ult ld va ossingmpreus "Coatint_st
    prkupsult bacd vass ol Compre  #
  le"
    "$temp_fi  rm -f     fi
    
  file"
p_< "$tem     done      fi
      ")"
    file"$name $(baseplicate: emoved duus "R  print_stat             + size))
 otal_saved (t$(aved=  total_s              ile"
 "$f    rm          "0")
   || echo /dev/null"$file" 2>$(stat -c%s ize=al sloc            hen
    $file" ]; t" -f         if [o
    ile; dash f read -r hhile    w
    occurrence)ep first ke (catesemove dupli     # R
         es"
  ilduplicate ft cate_coundupli "Found $print_status
        t 0 ]; then_count" -gte$duplica")
    if [ le"p_fi "$tem-l <_count=$(wc cateupli d
    localile"
    > "$temp_f32 -d -w sort | uniq   | \
      /dev/null m {} \; 2>sud5pe f -exec mts" -tyA_DIR/vaul$USER_DAT
    find "$"ates_$_duplicomvaultphant"/tmp/=file local temp_  licates
 entify dupmd5sum to ide find and  
    # Us  es..."
 e filplicating for ducannstatus "Sint_pr
     vaultss inicate file remove dupl# Find and   =0
    
 tal_savedocal to   
    l
  fi   return
 "
        optimizetod  vaults founatus "Norint_st
        pthen" ]; aultsDATA_DIR/vER_US ! -d "$ 
    if [ge"
   orang Vault Sttimizi "Oprint_header   p
 () {ltse_vauptimiz storage
oe vaultimizOpt
# fi
}
"
    nedcleaed: $es cleanotal filuccess "Tnt_s       prie
   elsean"
  o clary files tpors "No tem_succes     print
    ]; then" -eq 0"$cleanedf [  
    i   fi
   i
         f))
 + old_logsaned$((cleed=     clean"
       iles fogold lld_logs aned $oCle "success print_           true
/null ||  2>/devdelete30 -me +*.log" -mtis" -name "logATA_DIR/ "$USER_D  find
          ent 0 ]; th" -gd_logs if [ "$ol   l)
    | wc -2>/dev/null  +30 g" -mtimeme "*.los" -naDIR/log$USER_DATA_gs=$(find "l old_lo       loca then
 logs" ];IR/ATA_DSER_D-d "$U   if [ 0 days)
 der than 3 files (old log Clean ol #
    
      fifi
     )
    p_files)user_tem(cleaned + leaned=$(         c
   "ary files temporeruss _temp_fileeaned $user"Cl_success nt  pri     "/*
     _DIR/temp"$USER_DATA  rm -rf           en
t 0 ]; thes" -gser_temp_fil if [ "$u      wc -l)
  l |/dev/nulype f 2>R/temp" -t_DATA_DI"$USERles=$(find fiser_temp_cal u    lo    ]; then
R/temp" ATA_DI "$USER_D
    if [ -desfil temp ser # Clean u 
   i
   
    f        fifiles))
temp_cleaned + leaned=$((       c"
     ps from /tmrary file tempoilesned $temp_fss "Cleasucce    print_  e
      rul || t2>/dev/nul -delete t*" -type fhantomvaulp -name "*pnd /tm     fi   hen
     -gt 0 ]; tmp_files" "$te  if [)
       | wc -l/nullf 2>/devlt*" -type "*phantomvau -name nd /tmp_files=$(fical temp       loen
 thmp" ]; "/t-d    if [ 
 m temp filesystelean s C  
    #leaned=0
     local c
    
 y Files"oraraning Tempr "Cleheade  print_les() {
  n_temp_fis
cleary filetempora

# Clean   fi
}t 1
   exi       udo)"
use seges (root privilres uienance reqaintystem m "Sprint_error       then
  ystem" ]];$1" == "s] && [[ " -ne 0 "UID  if [ "$E) {
  es(rivilegck_prations
chestem operoot for syrunning as 
# Check if vault"
E/.phantomHOMA_DIR="$ER_DATUS
tomvault"phanICE_NAME="ault"
SERV/phantomvDIR="/optSTALL_guration
IN# Confi; }

]${NC} $1"}[ERRORe "${REDr() { echo -nt_erro"; }
pri]${NC} $1LLOW}[WARNo -e "${YE) { echint_warning(}
pr} $1"; NCEN}[DONE]${${GREo -e "echcess() { rint_suc$1"; }
pNT]${NC} E}[MAI"${BLUo -e tus() { echt_sta"; }
prin===${NC}=== $1 UE}BL\n${ "ho -er() { echeade
print_
[0m''
NC='\033033[0;34m\m'
BLUE='33OW='\033[1;
YELL033[0;32m'
GREEN='\31m'='\033[0;
REDlors Coe

#t -lities

seation utiptimizand o, e, cleanupenancSystem maint
# e ToolenancMaintantomVault 

# Phin/bashOF'
#!/be.sh" << 'Etenancult-maine/phantomvatenancIR/main> "$TOOLS_D
    cat "
    ance tool...aintenreating m"Cint_status  {
    prool()nance_ttee_mainate tool
creintenancate ma# Cre
d"
}
ol createostic toiagnem d "Systcessrint_suc
    p.sh"gnosticsdiatomvault-ics/phanagnostOOLS_DIR/dimod +x "$T
    ch
    EOF"

main "$@osticsagn Run dimary
}

#um  generate_srces
   check_resou_data
    check_useries
   ck_dependenc che  ork
 ck_netw
    cheicerv check_selation
   ck_instal
    chein() {
maecutionin ex
# Ma}
"
OG_FILEfile: $Lthis log provide  please r support,ho "Fo
    echo ""
    ec fi
    i
      f     nding"
poot ress nult API iantomVaerror "Ph    print_   e
             els"
ponding resI isult APVaPhantomccess "print_su            hen
"200" ]; t" = pi_status[ "$a if )
       "cho "000 || eev/nullll 2>/dv/nu/de" -o tformpla9876/api/t:localhosp://e}" "htthttp_codl -s -w "%{uri_status=$(c local ap
       then /dev/null; &>l and -v curomm if c  I status
  # APi
    
   "
    ft runningnois t service ntomVaul "Pharor_er  print   
   
    elsenning"vice is ruult serPhantomVass "cceint_su       pren
 l; th/nul &> /devltf phantomvaugrep -ll || pnu&> /dev/ICE_NAME" "$SERVe tl is-activsystemcif    s
 tue staServic  
    # fi
  
    "ngsimisomplete or tion is incstallaomVault inror "Phant    print_er  else
    lled"
  tay insproperlis ault hantomVcess "Pprint_sucn
        " ]; thehantomvaultin/pALL_DIR/b-f "$INST if [ 
   tusallation stanst   # I
     "
gs:"Key findin  echo 
   """
    echo$LOG_FILE to: edg savho "Full lo
    ece)"ed at: $(dattic completho "Diagnos  
    ecy"
  ic Summarstder "Diagnorint_hea) {
    pte_summary(eneraort
gy repsummarrate 
# Gene}
avg"
age: $load_ averloadem s "Syst_statunt  prin")
  ow "unkn|| echor -d ','  | t $1}''{print awk ' |rint $2}:' '{perageF'load ave | awk -vg=$(uptimad_a   local lod
  CPU loa Check
    #    ge"
usa: $disk_aceisk spvailable datus "At_st")
    prinnknown || echo "urint $4}'2{pNR==wk '/null | a>/devL_DIR" 2$INSTAL=$(df -h "_usagesklocal di
    cespa Check disk 
    # fi
    }MB"
   : ${free_memry available memoning "Lowrint_war      p ]; then
  nown"m" != "unk "$free_me [life"
    ey availablent memors "Sufficit_succesrin  p   en
   ]; tht 100 e_mem" -g&& [ "$frenown" ] = "unkm" !free_mef [ "$
    i"
    MBree_mem} memory: ${filabletatus "Avaint_spr"
    }MBtal_memy: ${total memor"Tostatus int_
    pr")"unknown}' || echo m:/{print $7'/^Meull | awk ev/n>/d-m 2ree $(free_mem=  local fwn")
  "unkno2}' || echo :/{print $k '/^Memev/null | awee -m 2>/dtal_mem=$(fr   local to
 y Check memor 
    #ck"
   ces Che Resouremader "Systt_herin pces() {
   check_resoursources
tem rek sys

# Checi
} f"
    groupantomvaults not in ph "User ingint_warni    pr
    "
    elseault grouptomv is in phan"Userss int_succe      prhen
  omvault; t-q phantups | grep  if gro group
   antomvaultser is in phheck if u# C
    
    r"
    finer: $owneory owation directus "Installtat  print_s     nknown")
 "u|| echo >/dev/null TALL_DIR" 2:%G" "$INS-c "%Uer=$(stat cal ownlo              
  "
ions: $permsory permission directllatatus "Instaint_st  pr
      ")wnecho "unknov/null || _DIR" 2>/deSTALLa" "$IN "%tat -c$(sperms=local   en
      " ]; thTALL_DIR"$INSif [ -d ns
    issiok perm  # Chec  
    
"
    fitallation)w insmal for neorund (ny not fotorr data direc"Useus stat  print_     lse
 fi
    e"
        ry foundlogs directostatus "No    print_
                 elseg_count"
s found: $lo files "Logtatunt_spri             wc -l)
/null |>/dev"*.log" 2-name r/logs" a_diuser_datfind "$=$(countal log_   loc        
  then/logs" ];ser_data_dir[ -d "$u  if gs
        # Check lo    
        fi
    
      d"ctory founfiles direro p_status "No    print    else
     "
       ntle_coufi $pro found:"Profilesstatus   print_         
 c -l)/null | wev>/d 2e "*.json"ames" -nofila_dir/pr"$user_datt=$(find ile_coun  local prof      ; then
    es" ]_dir/profilser_data [ -d "$u     if
   ck profilesChe
        #       "
  data_dir: $user_xists e directoryer datas "Usprint_succes   en
      thr" ];r_data_di-d "$use if [ "
   ntomvault"$HOME/.phair=_der_data usalloc   ctory
 ire data d# Check user
      k"
   ChecPermissionsnd ser Data aheader "U
    print_a() {ck_user_datns
chermissioa and pe datheck user

# C
    fi
}required)"e (x86_64 chitecturpported ar"Unsuning  print_war
       se elre"
   d architectuupportess "Succe    print_s  
  en" ]; th64"x86_ch" =  [ "$ar
    if    "
ch$architecture: em ar"Systatus print_st    (uname -m)
local arch=$e
    itecturystem arch# Check s
    
    "
    fiot foundmmand nSSL cong "Openni  print_war
      se"
    elversion: $ssl_sion verSLpenS"Ont_success  pri       ")
own echo "unkndev/null || 2>/ersionl vnssrsion=$(opecal ssl_veloen
        ull; thev/nl &> /dssopen -v ommandn
    if cversioL OpenSSheck    
    # C
    done
      fi$lib"
   t found:  noLibrary_warning "    print       lse
         elib"
und: $ fo"Librarys succes    print_en
        "; th "$lib | grep -q 2>/dev/nullonfig -p    if ldc  
  [@]}"; doquired_libs{reb in "$   for li")
    
 c++.so.6o" "libstdypto.slibcrssl.so" "6" "libo."libc.sed_libs=(local requirraries
    required libheck  # C  
     eck"
ndencies Chystem Depeader "Sprint_he   ncies() {
 eck_dependedencies
chem depensyst
# Check fi
}
"
    APIst  cannot tewget) -curl/ound (t flien"No HTTP cwarning  print_      e
 
    elsi"
        fndingt respono is HTTP APIr "erro print_       e
     els   ing"
    sponds reTP API i "HT_success    print     ; then
   >/dev/null" 2tform/pla/apit:$ipc_portalhosttp://locspider "het -q --f wg      i 
  "
       t...ion with wgenecton HTTP API cs "Testingatunt_st
        prihenev/null; twget &> /d-v mand f comelifi
            esponse)"
 $rnding (HTTPsponot reAPI is "HTTP nt_error pri         lse
          esponse)"
 HTTP $reing (ndI is respoHTTP APccess "nt_su    pri      
  " ]; then200onse" = "spreif [ "$
         "000")l || echodev/nulv/null 2>/m" -o /deorlatfort/api/phost:$ipc_pal"http://locde}" {http_corl -s -w "%$(cual response=       loc  
 
      ..."nection API coning HTTPtus "Testnt_sta        prinull; then
&> /dev/-v curl and f comm   iection
  connTTPest H
    # T    "
    fi
eningis not list $ipc_port C portg "IPninar   print_w
        else
 stening"is li$ipc_port IPC port s "ccesnt_supri  
      henort "; tipc_p":$ -q  grepv/null |at -ln 2>/dest net 
    ifrt..."
   $ipc_pog IPC port "Checkins statuprint_    876"
t="9pc_porl ioca
    lrtck IPC po Che   # 
 eck"
   nectivity Chetwork Cont_header "N   printwork() {
 
check_neconnectivityk Check networ
#     fi
}
 found"
ault processntomVPhar "No t_erro    prin
    else  done
    
      id ($cmd)"cess: PID $pPros "turint_sta          p
  ")"unknown || echo /null2>/devcomm= pid" -o =$(ps -p "$l cmdca lo          ids; do
 or pid in $p)
        fntomvaultep -f phaids=$(pgr local p
           
    "runningprocess is ult "PhantomVass _succe   print
     null; thent &> /dev/vaulp -f phantomgreif p    
 directlyprocessck  # Che
    
   )"
    fi/launchd(systemder found ce manag "No servirningnt_wa  pri    se
  
    el          fi
 ed"
     s not loadice i"Serving print_warn       e
             elsaded"
is loce erviss "Ssucce print_          "; then
 omvault"phantep -q | grnchctl list     if lau   
        
 ..."chd serviceng laun "Checkit_status  prinhen
      ; tnullv/hctl &> /de-v launcd if comman)
    elrvice (macOSk launchd seec  
    # Ch  logs"
    ervice e st retrievld no"Couint_warning pr| v/null |-n 5 2>/deager -no-pE" -ICE_NAMu "$SERVctl -al  journ      :"
ce logsecent servius "Rat   print_st    logs
  eck service        # Ch        

      fing"
   runni notService isrror "    print_e     
   e  els   fi
             "
  {cpu}%e: $"CPU usagint_status     pr            ")
 "0ll || echo/nu2>/devp "$pid"  -o %cpu= -cal cpu=$(ps          lo
      U usage CP   # Check          
                } KB"
    ${memoryy usage:s "Memornt_statu      pri          cho "0")
/null || e 2>/dev"$pid"s= -p $(ps -o rsy=cal memor        loe
        ory usagCheck mem#                   
            "
  id PID: $pviceccess "Serint_su         pr    en
   " ]; th" != "0id&& [ "$p"$pid" ]    if [ -n )
         ME"ERVICE_NA"$Svalue  MainPID -- show -ptlstemcl pid=$(sy        locaPID
    ervice  # Get s     
               
   running"ce is ess "Servi print_succ         then
   > /dev/null;_NAME" &VICE "$SERive-actystemctl is      if s   
       fi
        nabled"
e is not e"Servicint_warning pr        else
        d"
    ableenervice is ss "S print_succe       en
    ll; thev/nuNAME" &> /d"$SERVICE_ ledmctl is-enabsyste  if 
              ice..."
md servking systeatus "Chec    print_st
    thendev/null;  /l &>-v systemctand comm  if  (Linux)
  temd service# Check sys    k"
    
Chec
  
che

# Ch
