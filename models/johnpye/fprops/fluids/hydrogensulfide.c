/* This file is created by Hongke Zhu, 02-03-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE \
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "../helmholtz.h"

#define HYDROGENSULFIDE_M 34.08088 /* kg/kmol */
#define HYDROGENSULFIDE_R (8314.472/HYDROGENSULFIDE_M) /* J/kg/K */
#define HYDROGENSULFIDE_TSTAR 373.1 /* K */

const IdealData ideal_data_hydrogensulfide = {
    -4.0740770957 /* constant, a_1, adjust to solver s */
    , 3.7632137341 /* linear, a_2, adjust to solver h */
    , HYDROGENSULFIDE_TSTAR /* Tstar */
    , HYDROGENSULFIDE_R /* cp0star */
    , 2 /* power terms */
    , (const IdealPowTerm[]){
        {4.0,	0.0}
        ,{0.14327E-5,	1.5}
    }
    , 2 /* exponential terms */
    , (const IdealExpTerm[]){
        {1.1364,	1823.0}
        ,{1.9721,	3965.0}
    }
};

const HelmholtzData helmholtz_data_hydrogensulfide = {
	"hydrogensulfide"
    , /* R */ HYDROGENSULFIDE_R /* J/kg/K */
    , /* M */ HYDROGENSULFIDE_M /* kg/kmol */
    , /* rho_star */ 10.19*HYDROGENSULFIDE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ HYDROGENSULFIDE_TSTAR /* K (= T_c for this model) */

    , /* T_c */ HYDROGENSULFIDE_TSTAR
    , /* rho_c */ 10.19*HYDROGENSULFIDE_M /* kg/m3 */
    , /* T_t */ 0

    , 0.1005 /* acentric factor */
    , &ideal_data_hydrogensulfide
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.87641,	0.25,	1.0,	0.0}
        , {-2.0367,	1.125,	1.0,	0.0}
        , {0.21634,	1.5,	1.0,	0.0}
        , {-0.050199,	1.375,	2.0,	0.0}
        , {0.066994,	0.25,	3.0,	0.0}
        , {0.00019076,	0.875,	7.0,	0.0}
        , {0.20227,	0.625,	2.0,	1.0}
        , {-0.0045348,	1.75,	5.0,	1.0}
        , {-0.22230,	3.625,	1.0,	2.0}
        , {-0.034714,	3.625,	4.0,	2.0}
        , {-0.014885,	14.5,	3.0,	3.0}
        , {0.0074154,	12.0,	4.0,	3.0}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py hydrogensulfide
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_hydrogensulfide, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_hydrogensulfide, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_hydrogensulfide, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_hydrogensulfide, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_hydrogensulfide, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_hydrogensulfide, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_hydrogensulfide, ntd, td);
    return helm_run_test_cases(&helmholtz_data_hydrogensulfide, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-5.0E+1, 1.E-1, 1.87324735654E+0, 5.0365704935E+2, 5.57040282296E+2, 2.61897019638E+0, 7.56888201186E-1, 1.02391203839E+0, 9.82262585663E-1, -8.07661499732E+1}
    , {0.E+0, 1.E-1, 1.5157626897E+0, 5.4181275627E+2, 6.07786144241E+2, 2.82420441648E+0, 7.56484938817E-1, 1.01133581543E+0, 9.93120419515E-1, -2.29618680091E+2}
    , {5.0E+1, 1.E-1, 1.2759010661E+0, 5.80127642014E+2, 6.58503624806E+2, 2.99469275958E+0, 7.69254427824E-1, 1.01943093336E+0, 1.00975050261E+0, -3.87607323244E+2}
    , {1.00E+2, 1.E-1, 1.10255909441E+0, 6.19174196082E+2, 7.09872282477E+2, 3.14246548619E+0, 7.88349841422E-1, 1.03630020705E+0, 1.03045158188E+0, -5.53436800089E+2}
    , {1.50E+2, 1.E-1, 9.71071145592E-1, 6.59225849256E+2, 7.62204915675E+2, 3.27405015067E+0, 8.10786607444E-1, 1.05751830695E+0, 1.05364856477E+0, -7.26188472E+2}
    , {2.00E+2, 1.E-1, 8.67781423292E-1, 7.0042643718E+2, 8.15662828875E+2, 3.3934353717E+0, 8.3509631764E-1, 1.08109170036E+0, 1.07835536023E+0, -9.05177508939E+2}
    , {2.50E+2, 1.E-1, 7.84443581421E-1, 7.42858386206E+2, 8.70337279996E+2, 3.50326164431E+0, 8.60553234241E-1, 1.10607015571E+0, 1.10403653459E+0, -1.08987294301E+3}
    , {3.00E+2, 1.E-1, 7.15760728181E-1, 7.86573155045E+2, 9.26284648094E+2, 3.60538033788E+0, 8.86753123643E-1, 1.13194147809E+0, 1.13037145449E+0, -1.27985058561E+3}
    , {3.50E+2, 1.E-1, 6.58166908157E-1, 8.31603251067E+2, 9.8354039461E+2, 3.70114212192E+0, 9.13399327999E-1, 1.15835214005E+0, 1.15710345463E+0, -1.47476346221E+3}
    , {4.00E+2, 1.E-1, 6.09170230341E-1, 8.7796603868E+2, 1.04212376507E+3, 3.79155927096E+0, 9.40219472289E-1, 1.18499756035E+0, 1.18398067248E+0, -1.67432208456E+3}
    , {4.50E+2, 1.E-1, 5.66975393009E-1, 9.25664821041E+2, 1.10203931848E+3, 3.87740506131E+0, 9.66948982883E-1, 1.2115938236E+0, 1.21074968682E+0, -1.87828064904E+3}
    , {-5.0E+1, 1.E+0, 9.3177299333E+2, 2.00106491547E+1, 2.10838719323E+1, 9.23491120202E-2, 1.20134014266E+0, 1.99260836803E+0, 9.82262585663E-1, -5.97055192618E-1}
    , {0.E+0, 1.E+0, 1.68736653185E+1, 5.26968736789E+2, 5.86232683366E+2, 2.20656312727E+0, 8.3797883608E-1, 1.24341525529E+0, 9.93120419515E-1, -7.57539814254E+1}
    , {5.0E+1, 1.E+0, 1.35147089544E+1, 5.70628460538E+2, 6.44621914882E+2, 2.40313750341E+0, 8.02817253275E-1, 1.12201799782E+0, 1.00975050261E+0, -2.05945423689E+2}
    , {1.00E+2, 1.E+0, 1.14187041738E+1, 6.12277918853E+2, 6.99853530305E+2, 2.56210078276E+0, 8.05788141264E-1, 1.09442746587E+0, 1.03045158188E+0, -3.43769988235E+2}
    , {1.50E+2, 1.E+0, 9.93462685653E+0, 6.53853843916E+2, 7.54511877121E+2, 2.69956183327E+0, 8.20956124836E-1, 1.09471205929E+0, 1.05364856477E+0, -4.88465745832E+2}
    , {2.00E+2, 1.E+0, 8.81271114969E+0, 6.96045438921E+2, 8.09517897397E+2, 2.82241690344E+0, 8.4154606718E-1, 1.10686104934E+0, 1.07835536023E+0, -6.39381118943E+2}
    , {2.50E+2, 1.E+0, 7.92860775763E+0, 7.39170105866E+2, 8.65295654079E+2, 2.93446560481E+0, 8.64926009094E-1, 1.12496607346E+0, 1.10403653459E+0, -7.95995575291E+2}
    , {3.00E+2, 1.E+0, 7.21117865073E+0, 7.83395684284E+2, 9.22069269898E+2, 3.03809576039E+0, 8.8988427413E-1, 1.14639472097E+0, 1.13037145449E+0, -9.57888900783E+2}
    , {3.50E+2, 1.E+0, 6.61598015329E+0, 8.28818131297E+2, 9.79967315065E+2, 3.13493374454E+0, 9.15745403315E-1, 1.16977094145E+0, 1.15710345463E+0, -1.12471583161E+3}
    , {4.00E+2, 1.E+0, 6.11349326658E+0, 8.75492032813E+2, 1.03906463466E+3, 3.22614530807E+0, 9.42045090508E-1, 1.19425131768E+0, 1.18398067248E+0, -1.29618768132E+3}
    , {4.50E+2, 1.E+0, 5.68320273164E+0, 9.23443843497E+2, 1.09940096613E+3, 3.31259474427E+0, 9.68415272195E-1, 1.2192475009E+0, 1.21074968682E+0, -1.47205904582E+3}
    , {-5.0E+1, 1.E+1, 9.40834700981E+2, 1.60527082436E+1, 2.66815679036E+1, 7.43612884057E-2, 1.20580305256E+0, 1.96778142757E+0, 9.82262585663E-1, -5.4101326411E-1}
    , {0.E+0, 1.E+1, 8.49235935628E+2, 1.14068848807E+2, 1.2584413949E+2, 4.75152395202E-1, 1.11220122594E+0, 2.02160091932E+0, 9.93120419515E-1, -1.57190279423E+1}
    , {5.0E+1, 1.E+1, 7.34074042237E+2, 2.18657951481E+2, 2.32280555502E+2, 8.32360174584E-1, 1.05954657999E+0, 2.2972217619E+0, 1.00975050261E+0, -5.03192389362E+1}
    , {1.00E+2, 1.0E+1, 5.05535742123E+2, 3.64429764406E+2, 3.84210759431E+2, 1.26539507837E+0, 1.10457483015E+0, 5.98336395131E+0, 1.03045158188E+0, -1.07752409086E+2}
    , {1.50E+2, 1.0E+1, 1.37957367007E+2, 5.80190817455E+2, 6.52676979064E+2, 1.95677249853E+0, 9.55885634373E-1, 2.02843950792E+0, 1.05364856477E+0, -2.47817465296E+2}
    , {2.00E+2, 1.0E+1, 1.0562622686E+2, 6.4396578618E+2, 7.38639242738E+2, 2.14928144777E+0, 9.16155808484E-1, 1.53047505658E+0, 1.07835536023E+0, -3.72966730831E+2}
    , {2.50E+2, 1.0E+1, 8.87397368412E+1, 6.98197424434E+2, 8.10886512276E+2, 2.29455658786E+0, 9.12822589133E-1, 1.3829898613E+0, 1.10403653459E+0, -5.02199854504E+2}
    , {3.00E+2, 1.0E+1, 7.75931811691E+1, 7.49468339899E+2, 8.78345643926E+2, 2.41774954333E+0, 9.23188731018E-1, 1.32393844923E+0, 1.13037145449E+0, -6.3626481086E+2}
    , {3.50E+2, 1.0E+1, 6.94277907562E+1, 7.99830115016E+2, 9.43864656387E+2, 2.52736283125E+0, 9.40251738003E-1, 1.30083407533E+0, 1.15710345463E+0, -7.75096033279E+2}
    , {4.00E+2, 1.0E+1, 6.30777319653E+1, 8.50188089526E+2, 1.00872264187E+3, 2.62748135649E+0, 9.6088575145E-1, 1.29563338853E+0, 1.18398067248E+0, -9.18500985596E+2}
    , {4.50E+2, 1.0E+1, 5.79431486852E+1, 9.01008424816E+2, 1.07359138289E+3, 2.72043425408E+0, 9.83419022112E-1, 1.30035235577E+0, 1.21074968682E+0, -1.06627360602E+3}
    , {0.E+0, 1.00E+2, 9.43795228024E+2, 7.36199078848E+1, 1.79575095017E+2, 3.06586395767E-1, 1.15283595016E+0, 1.79218069803E+0, 9.93120419515E-1, -1.01241661189E+1}
    , {5.0E+1, 1.E+2, 8.82145074508E+2, 1.54995336294E+2, 2.68355375237E+2, 6.05125324282E-1, 1.09529222069E+0, 1.76329674974E+0, 1.00975050261E+0, -4.05509122477E+1}
    , {1.00E+2, 1.E+2, 8.20664886841E+2, 2.34339995387E+2, 3.56192412377E+2, 8.57876274999E-1, 1.06074702211E+0, 1.75186555704E+0, 1.03045158188E+0, -8.57765366288E+1}
    , {1.50E+2, 1.00E+2, 7.59937852721E+2, 3.12026117364E+2, 4.43615825183E+2, 1.07774841806E+0, 1.04132724873E+0, 1.7450305468E+0, 1.05364856477E+0, -1.4402312574E+2}
    , {2.00E+2, 1.00E+2, 7.01081626414E+2, 3.88001734953E+2, 5.30638478283E+2, 1.27214261166E+0, 1.03230447017E+0, 1.73494473317E+0, 1.07835536023E+0, -2.13912541753E+2}
    , {2.50E+2, 1.00E+2, 6.45418646104E+2, 4.62066550562E+2, 6.1700474549E+2, 1.44567609495E+0, 1.03071336585E+0, 1.71865952342E+0, 1.10403653459E+0, -2.94238898513E+2}
    , {3.00E+2, 1.00E+2, 5.94076727997E+2, 5.34086922797E+2, 7.02415347876E+2, 1.6016153808E+0, 1.03459742391E+0, 1.69708816873E+0, 1.13037145449E+0, -3.83878932706E+2}
    , {3.50E+2, 1.00E+2, 5.4772268844E+2, 6.04096937845E+2, 7.86671080035E+2, 1.74257211583E+0, 1.04260215125E+0, 1.67296605126E+0, 1.15710345463E+0, -4.81786876136E+2}
    , {4.00E+2, 1.00E+2, 5.06521960021E+2, 6.72291937259E+2, 8.69716743867E+2, 1.8707748056E+0, 1.05373602521E+0, 1.64910413995E+0, 1.18398067248E+0, -5.87020123131E+2}
    , {4.50E+2, 1.00E+2, 4.7025356507E+2, 7.3896963943E+2, 9.51620871506E+2, 1.98815015503E+0, 1.06722286549E+0, 1.62757429391E+0, 1.21074968682E+0, -6.98761145181E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
