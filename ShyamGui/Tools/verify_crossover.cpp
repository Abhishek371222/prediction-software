// Checks the engine's filter maths against the supplied MATLAB reference.
//
// Three .m files came with this work. Only one of them describes something a
// single cabinet can do, and that one is now a feature:
//
//   BandPass.m        H = HLP .* HHP   - one signal through BOTH ends in
//                     series. That is a band pass on one cabinet, and it is
//                     SpeakerFilter::Type::BandPass.
//
//   Crossover.m       H = HLP + pol*HHP - two signals, one through each end,
//                     ADDED. Nothing in a single cabinet does that: the sum
//                     happens in the air, between a low-passed box and a
//                     high-passed one, and the engine already produces it by
//                     summing complex pressure at every point of the map.
//                     Adding it to the engine would be a second, parallel
//                     implementation of a thing the engine does physically -
//                     so it lives here, as the reference that sum is checked
//                     against.
//
//   BandCrossover.m   The same addition one level up: band A + band B around
//                     a shared corner. Same reasoning, same place.
//
// Build (from ShyamGui/):
//   cl /std:c++17 /EHsc /I Source Tools\verify_crossover.cpp /Fe:verify_crossover.exe
//
// This file is NOT part of the application target.

#include "AcousticEngine.h"
#include <cmath>
#include <complex>
#include <cstdio>
#include <string>
#include <vector>

using cplx = std::complex<double>;
static constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// The MATLAB, transcribed line for line. Deliberately not sharing code with
// AcousticEngine.h - a reference that calls the thing it is checking proves
// nothing.
// ---------------------------------------------------------------------------
static const double a1 = 0.7653668647;
static const double a2 = 1.8477590650;

// Crossover.m, low-cut half: a LOW PASS at fl.
static cplx mLowPass (double f, const char* type, double fl, int Nl)
{
    const cplx s (0.0, 2.0 * kPi * f);
    const double wl = 2.0 * kPi * fl;
    if (std::string (type) == "BW")
    {
        if (Nl == 2)
            return (wl * wl) / (s * s + std::sqrt (2.0) * wl * s + wl * wl);
        const cplx H1 = (wl * wl) / (s * s + a1 * wl * s + wl * wl);
        const cplx H2 = (wl * wl) / (s * s + a2 * wl * s + wl * wl);
        return H1 * H2;
    }
    if (Nl == 2)
    {
        const cplx H1 = wl / (s + wl);
        return H1 * H1;
    }
    const cplx H1 = (wl * wl) / (s * s + std::sqrt (2.0) * wl * s + wl * wl);
    return H1 * H1;
}

// Crossover.m, high-cut half: a HIGH PASS at fh.
//
// NOTE: BandPass.m's own BW / 4th-order branch writes wh^2 numerators here,
// which is a low pass, not a high pass - a copy-paste from the section above
// it. Crossover.m has s^2 in the same place and is the one that is right, so
// that is what is transcribed, and what the engine implements.
static cplx mHighPass (double f, const char* type, double fh, int Nh)
{
    const cplx s (0.0, 2.0 * kPi * f);
    const double wh = 2.0 * kPi * fh;
    if (std::string (type) == "BW")
    {
        if (Nh == 2)
            return (s * s) / (s * s + std::sqrt (2.0) * wh * s + wh * wh);
        const cplx H1 = (s * s) / (s * s + a1 * wh * s + wh * wh);
        const cplx H2 = (s * s) / (s * s + a2 * wh * s + wh * wh);
        return H1 * H2;
    }
    if (Nh == 2)
    {
        const cplx H1 = s / (s + wh);
        return H1 * H1;
    }
    const cplx H1 = (s * s) / (s * s + std::sqrt (2.0) * wh * s + wh * wh);
    return H1 * H1;
}

// BandPass.m:  H = HLP .* HHP
static cplx mBandPass (double f, const char* lowtype, double fl, int Nl,
                       const char* hightype, double fh, int Nh)
{
    return mLowPass (f, lowtype, fl, Nl) * mHighPass (f, hightype, fh, Nh);
}

// Crossover.m:  H = HLP + pol*HHP
static cplx mCrossover (double f, const char* lowtype, double fl, int Nl,
                        const char* hightype, double fh, int Nh, double pol)
{
    return mLowPass (f, lowtype, fl, Nl) + pol * mHighPass (f, hightype, fh, Nh);
}

// BandCrossover.m:  HA = BandPass(tX,fc, tA,fa);  HB = BandPass(tB,fb, tX,fc);
//                   H  = HA + pol*HB
static cplx mBandCrossover (double f,
                            const char* tA, double fa, int NA,
                            const char* tX, double fc, int NX,
                            const char* tB, double fb, int NB, double pol)
{
    const cplx HA = mBandPass (f, tX, fc, NX, tA, fa, NA);
    const cplx HB = mBandPass (f, tB, fb, NB, tX, fc, NX);
    return HA + pol * HB;
}

// ---------------------------------------------------------------------------
static int failures = 0;
static int checks   = 0;

// Tolerance is 1e-9, not 0. Two reasons, both known and both harmless:
//
//  - The engine groups its arithmetic differently from the .m (sections go
//    through a shared lambda), and intermediate terms run to wc^4 ~ 2.5e16 at
//    2 kHz, so the last digits of a ~1.0 result are not expected to agree bit
//    for bit.
//  - The engine carries the Butterworth 4th-order Q pair to full precision,
//    2*sin(pi/8) = 0.76536686473017954, where the .m files round it to
//    0.7653668647 - a deliberate difference of about 1e-10 relative.
//
// A real error - a wrong numerator, a swapped section, a missing square -
// shows up at 1e-1, not 1e-11, so this still catches everything it is for.
static double worst = 0.0;

static void expect (const char* what, cplx got, cplx want, double tol = 1.0e-9)
{
    ++checks;
    const double d = std::abs (got - want);
    if (d > worst) worst = d;
    if (! (d <= tol))
    {
        ++failures;
        std::printf ("  FAIL %-38s |got-want| = %.3e\n", what, d);
    }
}

static double dB (cplx h) { return 20.0 * std::log10 (std::max (std::abs (h), 1e-300)); }

int main()
{
    const double freqs[] = { 20, 31.5, 40, 63, 80, 98, 125, 198, 250, 400,
                             630, 1000, 1600, 2000, 4000, 8000, 16000 };
    const char*  fams[]  = { "BW", "LR" };
    const int    orders[] = { 2, 4 };
    const double corners[] = { 40, 100, 198, 800, 2000 };

    auto famOf = [] (const char* t)
    {
        return std::string (t) == "BW" ? SpeakerFilter::Family::Butterworth
                                       : SpeakerFilter::Family::LinkwitzRiley;
    };

    // 1. Single sections - the engine's own low pass / high pass.
    std::printf ("1. Low pass / high pass vs Crossover.m sections\n");
    for (const char* t : fams)
        for (int n : orders)
            for (double fc : corners)
                for (double f : freqs)
                {
                    SpeakerFilter lp; lp.type = SpeakerFilter::Type::LowPass;
                    lp.family = famOf (t); lp.order = n; lp.fcHz = (float) fc;
                    expect ("low pass", filterResponse (f, lp), mLowPass (f, t, fc, n));

                    SpeakerFilter hp; hp.type = SpeakerFilter::Type::HighPass;
                    hp.family = famOf (t); hp.order = n; hp.fcHz = (float) fc;
                    expect ("high pass", filterResponse (f, hp), mHighPass (f, t, fc, n));
                }

    // 2. Band pass - the new filter type, against BandPass.m's product.
    std::printf ("2. Band pass vs BandPass.m (H = HLP .* HHP)\n");
    for (const char* tLo : fams)
        for (const char* tHi : fams)
            for (int nLo : orders)
                for (int nHi : orders)
                {
                    const double fLo = 100.0, fHi = 2000.0;
                    SpeakerFilter bp;
                    bp.type   = SpeakerFilter::Type::BandPass;
                    bp.family = famOf (tLo); bp.order   = nLo; bp.fcHz   = (float) fLo;
                    bp.familyHi = famOf (tHi); bp.orderHi = nHi; bp.fcHiHz = (float) fHi;

                    for (double f : freqs)
                        expect ("band pass",
                                filterResponse (f, bp),
                                // fl = LP cutoff = the band's TOP, fh = HP
                                // cutoff = the band's BOTTOM. The .m file's
                                // "low"/"high" name the SECTIONS, not the ends.
                                mBandPass (f, tHi, fHi, nHi, tLo, fLo, nLo));
                }

    // 3. Crossover.m - the SUM, which the engine makes in the air. Checked as
    //    the identity it is: summing the engine's own two sections must equal
    //    the reference sum, for every family / order / polarity.
    std::printf ("3. Crossover.m sum vs summing the engine's two sections\n");
    for (const char* t : fams)
        for (int n : orders)
            for (double pol : { 1.0, -1.0 })
            {
                const double fc = 198.0;
                SpeakerFilter lp; lp.type = SpeakerFilter::Type::LowPass;
                lp.family = famOf (t); lp.order = n; lp.fcHz = (float) fc;
                SpeakerFilter hp; hp.type = SpeakerFilter::Type::HighPass;
                hp.family = famOf (t); hp.order = n; hp.fcHz = (float) fc;

                for (double f : freqs)
                    expect ("crossover sum",
                            filterResponse (f, lp) + pol * filterResponse (f, hp),
                            mCrossover (f, t, fc, n, t, fc, n, pol));
            }

    // 4. BandCrossover.m - two band passes around a shared corner, added.
    std::printf ("4. BandCrossover.m vs two engine band passes added\n");
    {
        const double fa = 60.0, fc = 500.0, fb = 4000.0;
        for (const char* tX : fams)
            for (int NX : orders)
            {
                SpeakerFilter A;                       // fa -> fc
                A.type = SpeakerFilter::Type::BandPass;
                A.family = famOf ("LR"); A.order = 4;   A.fcHz   = (float) fa;
                A.familyHi = famOf (tX); A.orderHi = NX; A.fcHiHz = (float) fc;

                SpeakerFilter B;                       // fc -> fb
                B.type = SpeakerFilter::Type::BandPass;
                B.family = famOf (tX); B.order = NX;    B.fcHz   = (float) fc;
                B.familyHi = famOf ("LR"); B.orderHi = 4; B.fcHiHz = (float) fb;

                for (double pol : { 1.0, -1.0 })
                    for (double f : freqs)
                        expect ("band crossover",
                                filterResponse (f, A) + pol * filterResponse (f, B),
                                mBandCrossover (f, "LR", fa, 4, tX, fc, NX,
                                                "LR", fb, 4, pol));
            }
    }

    // 5. The textbook numbers, so a pass here means something in decibels.
    std::printf ("5. Corner levels\n");
    {
        SpeakerFilter lr; lr.type = SpeakerFilter::Type::LowPass;
        lr.family = SpeakerFilter::Family::LinkwitzRiley; lr.order = 4; lr.fcHz = 200.0f;
        std::printf ("   LR4 low pass at its own corner : %+.2f dB (expect -6.02)\n",
                     dB (filterResponse (200.0, lr)));

        SpeakerFilter bw = lr; bw.family = SpeakerFilter::Family::Butterworth;
        std::printf ("   BW4 low pass at its own corner : %+.2f dB (expect -3.01)\n",
                     dB (filterResponse (200.0, bw)));

        SpeakerFilter hp; hp.type = SpeakerFilter::Type::HighPass;
        hp.family = SpeakerFilter::Family::LinkwitzRiley; hp.order = 4; hp.fcHz = 200.0f;
        std::printf ("   LR4 crossover sum at corner    : %+.2f dB (expect  0.00)\n",
                     dB (filterResponse (200.0, lr) + filterResponse (200.0, hp)));

        SpeakerFilter bp; bp.type = SpeakerFilter::Type::BandPass;
        bp.family = SpeakerFilter::Family::LinkwitzRiley; bp.order = 4; bp.fcHz = 100.0f;
        bp.familyHi = SpeakerFilter::Family::LinkwitzRiley; bp.orderHi = 4; bp.fcHiHz = 2000.0f;
        std::printf ("   LR4 band pass 100-2000 Hz, mid : %+.2f dB (expect ~0)\n",
                     dB (filterResponse (447.0, bp)));
        std::printf ("   ...at its lower corner         : %+.2f dB (expect ~-6)\n",
                     dB (filterResponse (100.0, bp)));
        std::printf ("   ...at its upper corner         : %+.2f dB (expect ~-6)\n",
                     dB (filterResponse (2000.0, bp)));

        // A band whose top is not above its bottom passes nothing, so it is
        // treated as off rather than silencing the cabinet.
        SpeakerFilter bad = bp; bad.fcHiHz = 50.0f;
        std::printf ("   inverted band (top below base): %s\n",
                     (std::abs (filterResponse (447.0, bad) - cplx (1.0, 0.0)) < 1e-15)
                        ? "treated as off - ok" : "FAIL");
        if (! (std::abs (filterResponse (447.0, bad) - cplx (1.0, 0.0)) < 1e-15)) ++failures;
    }

    std::printf ("\n%d checks, %d failures, largest difference %.3e\n",
                 checks, failures, worst);
    return failures == 0 ? 0 : 1;
}
