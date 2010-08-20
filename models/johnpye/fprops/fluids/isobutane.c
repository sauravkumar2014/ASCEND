/* This file is created by Hongke Zhu, 7-20-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Buecker, D. and Wagner, W.,
"Reference Equations of State for the Thermodynamic Properties of Fluid Phase n-Butane and Isobutane,"
J. Phys. Chem. Ref. Data, 35(2):929-1019, 2006.
*/

#include "../helmholtz.h"

#define ISOBUTANE_M 58.1222  /* kg/kmol */
#define ISOBUTANE_R (8314.472/ISOBUTANE_M) /* J/kg/K */
#define ISOBUTANE_TSTAR 407.81 /* K */

const IdealData ideal_data_isobutane = {
    11.60865546 /* constant */
    , -5.29450411 /* linear */
    , ISOBUTANE_TSTAR /* Tstar */
    , ISOBUTANE_R /* cp0star */
    , 1 /* power terms */
    , (const IdealPowTerm[]){
        {4.05956619,	0.0}
    }
    , 4 /* exponential terms */
    , (const IdealExpTerm[]){
        {4.94641014,    387.94064}
        ,{4.09475197,   973.80782}
        ,{15.6632824,   1772.71103}
        ,{9.73918122,   4228.52424}
    } 
};

const HelmholtzData helmholtz_data_isobutane = {
	"isobutane"
    , /* R */ ISOBUTANE_R /* J/kg/K */
    , /* M */ ISOBUTANE_M /* kg/kmol */
    , /* rho_star */ 3.879756788*ISOBUTANE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ ISOBUTANE_TSTAR /* K (= T_c for this model) */

    , /* T_c */ ISOBUTANE_TSTAR
    , /* rho_c */ 3.879756788*ISOBUTANE_M /* kg/m3 */
    , /* T_t */ 0

    , 0.184 /* acentric factor */
    , &ideal_data_isobutane
    , 23 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.20686820727966E+01,    0.5,    1.0,   0}
        , {-0.36400098615204E+01,    1.0,    1.0,  0}
	, {0.51968754427244E+00,    1.5,    1.0,   0}
	, {0.17745845870123E+00,    0.0,    2.0,   0}
	, {-0.12361807851599E+00,    0.5,    3.0,  0}
	, {0.45145314010528E-01,    0.5,    4.0,   0}
	, {0.30476479965980E-01,    0.75,   4.0,   0}
	, {0.75508387706302E+00,    2.0,    1.0,   1}
	, {-0.85885381015629E+00,    2.5,    1.0,  1}
	, {0.36324009830684E-01,    2.5,    2.0,   1}
	, {-0.19548799450550E-01,    1.5,    7.0,  1}
	, {-0.44452392904960E-02,    1.0,    8.0,  1}
	, {0.46410763666460E-02,    1.5,    8.0,   1}
	, {-0.71444097992825E-01,    4.0,    1.0,  2}
	, {-0.80765060030713E-01,    7.0,    2.0,  2}
	, {0.15560460945053E+00,    3.0,    3.0,   2}
	, {0.20318752160332E-02,    7.0,    3.0,   2}
	, {-0.10624883571689E+00,    3.0,    4.0,  2}
	, {0.39807690546305E-01,    1.0,    5.0,   2}
	, {0.16371431292386E-01,    6.0,    5.0,   2}
	, {0.53212200682628E-03,    0.0,   10.0,   2}
	, {-0.78681561156387E-02,    6.0,    2.0,  3}
	, {-0.30981191888963E-02,   13.0,    6.0,  3}
    }
    , 2 /* gaussian terms */
    , (const HelmholtzGausTerm[]){
        /* n, t, d, alpha, beta, gamma, epsilon */
        {-0.42276036810382E-01,    2.0,   1.0,  10.0, 150.0, 1.16, 0.85}
        , {-0.53001044558079E-02,   0.0,   2.0,  10.0, 200.0, 1.13, 1.0}
    }
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py isobutane
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_isobutane, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_isobutane, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_isobutane, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_isobutane, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_isobutane, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_isobutane, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_isobutane, ntd, td);
    return helm_run_test_cases(&helmholtz_data_isobutane, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.50E+2, 1.00000000001E-1, 7.31483347373E+2, -6.98014973578E+2, -6.97878265066E+2, -3.06334010257E+0, 1.19409447992E+0, 1.71867182316E+0, 9.2286450497E-1, -3.20764639946E+2}
    , {-1.00E+2, 9.99999999999E-2, 6.839596082E+2, -6.08095697022E+2, -6.07949489557E+2, -2.4520018503E+0, 1.30041649732E+0, 1.87799805547E+0, 1.12982278325E+0, -1.83531576643E+2}
    , {-5.0E+1, 1.E-1, 6.34514846852E+2, -5.09974741387E+2, -5.09817140662E+2, -1.95504036647E+0, 1.42444344821E+0, 2.05336292611E+0, 1.32999425432E+0, -7.37074836091E+1}
    , {0.E+0, 1.E-1, 2.65220633655E+0, -8.22883086602E+1, -4.45838515741E+1, -1.50010700299E-1, 1.42152290965E+0, 1.58958226796E+0, 1.54739989119E+0, -4.13128858736E+1}
    , {5.0E+1, 1.E-1, 2.20743465721E+0, -5.22831327823E+0, 4.00731409112E+1, 1.34098101159E-1, 1.64541141172E+0, 1.80172087223E+0, 1.78046207644E+0, -4.85621146677E+1}
    , {1.00E+2, 1.E-1, 1.89715886507E+0, 8.31219819086E+1, 1.35832380516E+2, 4.09231203501E-1, 1.87805101828E+0, 2.02926399907E+0, 2.01679746758E+0, -6.95826416776E+1}
    , {1.50E+2, 1.E-1, 1.66586403031E+0, 1.82905808285E+2, 2.42934717115E+2, 6.7829018881E-1, 2.10473530277E+0, 2.25328943157E+0, 2.24522801661E+0, -1.0411268511E+2}
    , {2.00E+2, 1.E-1, 1.48594566898E+0, 2.93657295276E+2, 3.60954506801E+2, 9.41692519145E-1, 2.3180826541E+0, 2.46508324507E+0, 2.45949268253E+0, -1.51904520157E+2}
    , {2.50E+2, 1.E-1, 1.34165389312E+0, 4.14651578432E+2, 4.89186449544E+2, 1.19916002674E+0, 2.51555917297E+0, 2.66157552702E+0, 2.65749257143E+0, -2.12688989558E+2}
    , {3.00E+2, 1.E-1, 1.22320433662E+0, 5.45101791329E+2, 6.26854281085E+2, 1.45035859003E+0, 2.69728510061E+0, 2.84263951075E+0, 2.83953655438E+0, -2.8617123455E+2}
    , {-1.50E+2, 9.99999999999E-1, 7.31834370763E+2, -6.98209693547E+2, -6.96843264148E+2, -3.0649241901E+0, 1.19546504905E+0, 1.7181671826E+0, 9.2286450497E-1, -3.20764279536E+2}
    , {-1.00E+2, 1.E+0, 6.84492842772E+2, -6.08414939046E+2, -6.0695400339E+2, -2.45384919507E+0, 1.30154573973E+0, 1.87685802795E+0, 1.12982278325E+0, -1.83530950919E+2}
    , {-5.0E+1, 1.E+0, 6.35339535411E+2, -5.10481226653E+2, -5.08907265103E+2, -1.95731510611E+0, 1.42548247352E+0, 2.05089208695E+0, 1.32999425432E+0, -7.37063607241E+1}
    , {0.E+0, 1.E+0, 5.81873391724E+2, -4.02715094352E+2, -4.00996507432E+2, -1.52173983384E+0, 1.5881252408E+0, 2.27700776939E+0, 1.54739989119E+0, 1.2948141262E+1}
    , {5.0E+1, 1.E+0, 5.18359818208E+2, -2.8148175082E+2, -2.79552588943E+2, -1.11422408027E+0, 1.78679938401E+0, 2.60749760388E+0, 1.78046207644E+0, 7.85797607186E+1}
    , {1.00E+2, 1.E+0, 2.17804276255E+1, 6.75749123644E+1, 1.13487693197E+2, 3.69635258329E-2, 1.92840020033E+0, 2.20306210137E+0, 2.01679746758E+0, 5.37819726999E+1}
    , {1.50E+2, 1.E+0, 1.81093330684E+1, 1.71692062936E+2, 2.26912208053E+2, 3.2202749355E-1, 2.13089436426E+0, 2.34584233968E+0, 2.24522801661E+0, 3.54261290403E+1}
    , {2.00E+2, 1.E+0, 1.56962760987E+1, 2.8489789512E+2, 3.48607273938E+2, 5.93674394526E-1, 2.33381857034E+0, 2.52359345365E+0, 2.45949268253E+0, 4.00085534974E+0}
    , {2.50E+2, 1.E+0, 1.39296704726E+1, 4.07485530621E+2, 4.79274737837E+2, 8.56050133302E-1, 2.52592179049E+0, 2.7021640614E+0, 2.65749257143E+0, -4.03570966154E+1}
    , {3.00E+2, 1.E+0, 1.25584678113E+1, 5.3905630246E+2, 6.18683850583E+2, 1.11043495111E+0, 2.70456365455E+0, 2.87253363193E+0, 2.83953655438E+0, -9.73894897698E+1}
    , {-1.50E+2, 1.E+1, 7.35272774896E+2, -7.00082040144E+2, -6.86481645377E+2, -3.08041154548E+0, 1.20868611618E+0, 1.71350513817E+0, 9.2286450497E-1, -3.20729358318E+2}
    , {-1.00E+2, 1.E+1, 6.8963540706E+2, -6.11451570759E+2, -5.9695115518E+2, -2.4717290491E+0, 1.31230219925E+0, 1.86667740794E+0, 1.12982278325E+0, -1.83471685907E+2}
    , {-5.0E+1, 1.E+1, 6.43090208212E+2, -5.15196969029E+2, -4.99647051036E+2, -1.97890670817E+0, 1.4353546877E+0, 2.03002807742E+0, 1.32999425432E+0, -7.36039371004E+1}
    , {0.E+0, 1.E+1, 5.94254411875E+2, -4.10141075265E+2, -3.93313265828E+2, -1.54962384388E+0, 1.59708005059E+0, 2.23077647372E+0, 1.54739989119E+0, 1.313867769E+1}
    , {5.0E+1, 1.E+1, 5.40885705318E+2, -2.94251544995E+2, -2.75763350721E+2, -1.15501700582E+0, 1.7915017786E+0, 2.48002057676E+0, 1.78046207644E+0, 7.89922004355E+1}
    , {1.00E+2, 1.E+1, 4.787717098E+2, -1.65149915949E+2, -1.44263134639E+2, -7.77192532952E-1, 2.00500328123E+0, 2.79329729104E+0, 2.01679746758E+0, 1.24859477722E+2}
    , {1.50E+2, 1.E+1, 3.99448463112E+2, -1.95950629726E+1, 5.4394556787E+0, -4.01262048806E-1, 2.2298815345E+0, 3.22095190339E+0, 2.24522801661E+0, 1.5019897298E+2}
    , {2.00E+2, 1.E+1, 2.93088171237E+2, 1.45575093921E+2, 1.79694519341E+2, -1.25555988288E-2, 2.44892921869E+0, 3.70065596898E+0, 2.45949268253E+0, 1.51515775507E+2}
    , {2.50E+2, 1.E+1, 2.03534672008E+2, 3.12131275516E+2, 3.61262953678E+2, 3.52438720432E-1, 2.61608065884E+0, 3.49154182784E+0, 2.65749257143E+0, 1.27752958922E+2}
    , {3.00E+2, 1.0E+1, 1.57800972116E+2, 4.67536512451E+2, 5.30907478206E+2, 6.62248629989E-1, 2.7663954814E+0, 3.33243610867E+0, 2.83953655438E+0, 8.79687101728E+1}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
