#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

class SantosLevelerEngine
{
public:
    struct Parameters
    {
        float targetDb = -20.0f;
        float gateDb = -45.0f;
        float speedMs = 15.0f;
        float detectMs = 8.0f;
        float lookaheadMs = 30.0f;
        float holdMs = 100.0f;
        float releaseMs = 500.0f;
        float peakThresholdDb = -9.0f;
        float rangeDownDb = -9.0f;
        float rangeUpDb = 9.0f;
        float outputDb = 0.0f;
    };

    struct Telemetry
    {
        float inputDb  = -100.0f;
        float riderDb  = 0.0f;
        float outputDb = -100.0f;
        bool riderActive = false;
    };

    void prepare (double newSampleRate, int newNumChannels)
    {
        sampleRate = std::max (1.0, newSampleRate);
        numChannels = std::clamp (newNumChannels, 1, 2);
        inputDetector.prepare (sampleRate, 100.0);
        outputDetector.prepare (sampleRate, 100.0);
        controlPeriodSamples = std::max (1, static_cast<int> (std::round (sampleRate / 240.0)));
        reset();
    }

    void reset()
    {
        inputDetector.reset();
        outputDetector.reset();
        controlCountdown = 0;
        currentRiderGain = 1.0f;
        targetRiderGain = 1.0f;
        currentOutputGain = 1.0f;
        latestRequestedCorrectionDb = 0.0f;
        heldCorrectionDb = 0.0f;
        holdSamplesRemaining = 0;
        holdActive = false;
        detectorActive = false;

        peakEnvelopeDb = -100.0f;
        peakReductionDb = 0.0f;
        currentPeakGain = 1.0f;
        riderActive = false;
        lastTelemetry = {};
    }

    Telemetry processSample(float& left, float& right, const Parameters& p)
    {
        const auto detectorLeft = left;
        const auto detectorRight = right;

        return processSampleLookahead(detectorLeft, detectorRight, left, right, p);
    }

    Telemetry processSampleLookahead(float detectorLeft,
        float detectorRight,
        float& left,
        float& right,
        const Parameters& p)
    {
        const auto detectMs = std::clamp(p.detectMs, 1.0f, 100.0f);

        inputDetector.setWindowMs(detectMs);
        outputDetector.setWindowMs(detectMs);

        // El detector analiza la señal sin retrasar.
        const auto inputRms =
            inputDetector.process(detectorLeft,
                detectorRight,
                numChannels);

        const auto inputDb = gainToDb(inputRms);


        // ------------------------------------------------------------
        // DETECTOR PEAK
        // ------------------------------------------------------------

        // Medimos el pico instantáneo de la señal original,
        // antes del Lookahead.
        const auto instantaneousPeak =
            numChannels > 1
            ? std::max(std::abs(detectorLeft),
                std::abs(detectorRight))
            : std::abs(detectorLeft);

        const auto instantaneousPeakDb =
            gainToDb(instantaneousPeak);


        // Ataque instantáneo:
        // si aparece un pico mayor, lo capturamos inmediatamente.
        if (instantaneousPeakDb > peakEnvelopeDb)
        {
            peakEnvelopeDb = instantaneousPeakDb;
        }
        else
        {
            // La lectura de pico cae más lentamente para evitar
            // que desaparezca inmediatamente después del transitorio.
            const auto peakReleaseAlpha =
                timeConstantAlpha(80.0f);

            peakEnvelopeDb +=
                peakReleaseAlpha
                * (instantaneousPeakDb - peakEnvelopeDb);
        }


        // ------------------------------------------------------------
        // CÁLCULO DEL RIDER
        // ------------------------------------------------------------

        if (controlCountdown <= 0)
        {
            const auto errorDb = p.targetDb - inputDb;

            const auto positive =
                std::clamp(errorDb, 0.0f, 12.0f)
                * (std::clamp(p.rangeUpDb, 0.0f, 12.0f) / 12.0f);

            const auto negative =
                std::clamp(errorDb, -12.0f, 0.0f)
                * (std::clamp(-p.rangeDownDb, 0.0f, 12.0f) / 12.0f);

            detectorActive = inputDb > p.gateDb;

            const auto newCorrectionDb =
                detectorActive
                ? positive + negative
                : 0.0f;

            latestRequestedCorrectionDb = newCorrectionDb;


            // --------------------------------------------------------
            // HOLD
            //
            // Si aparece una corrección MÁS fuerte, reaccionamos.
            //
            // Si la corrección intenta volver hacia 0 dB,
            // mantenemos temporalmente la corrección anterior.
            // --------------------------------------------------------

            constexpr float epsilonDb = 0.05f;

            const auto heldNearZero =
                std::abs(heldCorrectionDb) < epsilonDb;

            const auto newNearZero =
                std::abs(newCorrectionDb) < epsilonDb;

            const bool sameDirection =
                heldNearZero
                || newNearZero
                || ((heldCorrectionDb > 0.0f)
                    == (newCorrectionDb > 0.0f));

            const bool newIsAtLeastAsStrong =
                sameDirection
                && std::abs(newCorrectionDb)
                >= std::abs(heldCorrectionDb) - epsilonDb;


            if (newIsAtLeastAsStrong)
            {
                // Una corrección más fuerte siempre tiene prioridad.
                heldCorrectionDb = newCorrectionDb;

                holdActive = false;
                holdSamplesRemaining = 0;
            }
            else if (!holdActive)
            {
                // La corrección quiere relajarse.
                // Empezamos el tiempo de HOLD.
                const auto safeHoldMs =
                    std::clamp(p.holdMs, 0.0f, 1000.0f);

                holdSamplesRemaining =
                    static_cast<int> (
                        std::round(
                            sampleRate
                            * static_cast<double> (safeHoldMs)
                            * 0.001));

                holdActive = holdSamplesRemaining > 0;
            }

            controlCountdown = controlPeriodSamples;
        }

        --controlCountdown;


        // ------------------------------------------------------------
        // CONTADOR HOLD
        // ------------------------------------------------------------

        if (holdActive)
        {
            if (holdSamplesRemaining > 0)
                --holdSamplesRemaining;

            if (holdSamplesRemaining <= 0)
            {
                holdActive = false;
                heldCorrectionDb = latestRequestedCorrectionDb;
            }
        }


        // Mientras HOLD esté activo conservamos la corrección anterior.
        const auto effectiveCorrectionDb =
            holdActive
            ? heldCorrectionDb
            : latestRequestedCorrectionDb;

        if (!holdActive)
            heldCorrectionDb = latestRequestedCorrectionDb;


        targetRiderGain =
            dbToGain(effectiveCorrectionDb);


        // ------------------------------------------------------------
        // SPEED / RELEASE
        // ------------------------------------------------------------

        const auto currentRiderDb =
            gainToDb(currentRiderGain);

        constexpr float epsilonDb = 0.05f;

        const auto currentNearZero =
            std::abs(currentRiderDb) < epsilonDb;

        const auto targetNearZero =
            std::abs(effectiveCorrectionDb) < epsilonDb;

        const bool sameDirection =
            currentNearZero
            || targetNearZero
            || ((currentRiderDb > 0.0f)
                == (effectiveCorrectionDb > 0.0f));

        const bool movingAwayFromUnity =
            sameDirection
            && std::abs(effectiveCorrectionDb)
       > std::abs(currentRiderDb) + epsilonDb;


       // SPEED controla la velocidad para aplicar la corrección.
       const auto safeSpeedMs =
           std::clamp(p.speedMs, 2.0f, 250.0f);


       // RELEASE controla la vuelta hacia una corrección menor / 0 dB.
       const auto safeReleaseMs =
           std::clamp(p.releaseMs, 50.0f, 3000.0f);


       const auto riderAlpha =
           movingAwayFromUnity
           ? timeConstantAlpha(safeSpeedMs)
           : timeConstantAlpha(safeReleaseMs);


       currentRiderGain +=
           riderAlpha
           * (targetRiderGain - currentRiderGain);

       // ------------------------------------------------------------
       // PROTECCIÓN DE PICOS
       // ------------------------------------------------------------

       // El umbral PEAK es ahora independiente del TARGET.
       // No es un limitador True Peak: es el punto a partir del cual
       // entra en acción la protección rápida de transitorios.
       const auto peakThresholdDb =
           std::clamp(p.peakThresholdDb, -18.0f, -1.0f);


       // Calculamos dónde quedaría el pico después
       // del rider normal.
       const auto predictedPeakDb =
           peakEnvelopeDb + gainToDb(currentRiderGain);


       // Calculamos una reducción adicional,
       // limitada a un máximo de 9 dB.
       if (predictedPeakDb > peakThresholdDb)
       {
           peakReductionDb =
               std::clamp(
                   peakThresholdDb - predictedPeakDb,
                   -9.0f,
                   0.0f);
       }
       else
       {
           peakReductionDb = 0.0f;
       }


       // Convertimos la reducción PEAK a ganancia.
       const auto targetPeakGain =
           dbToGain(peakReductionDb);


       // Cuando aparece un pico, actuamos rápidamente.
       // Cuando desaparece, recuperamos suavemente.
       const bool needsPeakReduction =
           targetPeakGain < currentPeakGain;

       const auto peakAlpha =
           needsPeakReduction
           ? timeConstantAlpha(1.0f)
           : timeConstantAlpha(100.0f);


       currentPeakGain +=
           peakAlpha
           * (targetPeakGain - currentPeakGain);

           // ------------------------------------------------------------
           // OUTPUT
           // ------------------------------------------------------------

           const auto wantedOutputGain =
               dbToGain(
                   std::clamp(p.outputDb, -12.0f, 12.0f));

           const auto outputAlpha =
               timeConstantAlpha(30.0f);

           currentOutputGain +=
               outputAlpha
               * (wantedOutputGain - currentOutputGain);


           const auto totalGain =
               currentRiderGain
               * currentPeakGain
               * currentOutputGain;


           // La ganancia se aplica al audio retrasado por Lookahead.
           left *= totalGain;

           if (numChannels > 1)
               right *= totalGain;
           else
               right = left;


           const auto outputRms =
               outputDetector.process(left,
                   right,
                   numChannels);

           const auto outputDb =
               gainToDb(outputRms);


           // ------------------------------------------------------------
           // TELEMETRÍA
           // ------------------------------------------------------------

           lastTelemetry.inputDb = inputDb;
           lastTelemetry.riderDb =
               gainToDb(currentRiderGain);

           lastTelemetry.outputDb = outputDb;

           // Seguimos indicando RIDER activo durante Hold/Release.
           riderActive =
               detectorActive
               || std::abs(lastTelemetry.riderDb) > 0.05f;

           lastTelemetry.riderActive = riderActive;

           return lastTelemetry;
    }

    Telemetry getTelemetry() const noexcept { return lastTelemetry; }

    static float dbToGain (float db) noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }

    static float gainToDb (float gain) noexcept
    {
        if (gain <= 1.0e-8f)
            return -100.0f;
        return std::max (-100.0f, 20.0f * std::log10 (gain));
    }

private:
    class SlidingRms
    {
    public:
        void prepare (double sr, double maxWindowMs)
        {
            sampleRate = std::max (1.0, sr);
            maxSamples = std::max (1, static_cast<int> (std::ceil (sampleRate * maxWindowMs * 0.001)));
            history.assign (static_cast<std::size_t> (maxSamples), 0.0f);
            reset();
        }

        void reset()
        {
            std::fill (history.begin(), history.end(), 0.0f);
            writeIndex = 0;
            filled = 0;
            windowSamples = 1;
            sumSquares = 0.0;
        }

        void setWindowMs (float ms)
        {
            const auto newWindow = std::clamp (
                static_cast<int> (std::round (sampleRate * static_cast<double> (ms) * 0.001)),
                1, maxSamples);

            if (newWindow == windowSamples)
                return;

            windowSamples = newWindow;
            recomputeSum();
        }

        float process (float left, float right, int channels)
        {
            const auto square = channels > 1
                ? 0.5f * (left * left + right * right)
                : left * left;

            if (filled >= windowSamples)
            {
                const auto oldIndex = (writeIndex - windowSamples + maxSamples) % maxSamples;
                sumSquares -= static_cast<double> (history[static_cast<std::size_t> (oldIndex)]);
            }
            else
            {
                ++filled;
            }

            history[static_cast<std::size_t> (writeIndex)] = square;
            sumSquares += static_cast<double> (square);
            writeIndex = (writeIndex + 1) % maxSamples;

            const auto denominator = std::max (1, std::min (filled, windowSamples));
            return std::sqrt (static_cast<float> (std::max (0.0, sumSquares) / denominator));
        }

    private:
        void recomputeSum()
        {
            sumSquares = 0.0;
            const auto count = std::min (filled, windowSamples);
            for (int i = 1; i <= count; ++i)
            {
                const auto idx = (writeIndex - i + maxSamples) % maxSamples;
                sumSquares += history[static_cast<std::size_t> (idx)];
            }
        }

        double sampleRate = 48000.0;
        int maxSamples = 1;
        int windowSamples = 1;
        int writeIndex = 0;
        int filled = 0;
        double sumSquares = 0.0;
        std::vector<float> history;
    };

    float timeConstantAlpha (float ms) const noexcept
    {
        const auto seconds = std::max (0.000001, static_cast<double> (ms) * 0.001);
        return static_cast<float> (1.0 - std::exp (-1.0 / (seconds * sampleRate)));
    }

    double sampleRate = 48000.0;
    int numChannels = 2;
    int controlPeriodSamples = 200;
    int controlCountdown = 0;
    float targetRiderGain = 1.0f;
    float currentRiderGain = 1.0f;
    float currentOutputGain = 1.0f;
    bool riderActive = false;
    float latestRequestedCorrectionDb = 0.0f;
    float heldCorrectionDb = 0.0f;

    int holdSamplesRemaining = 0;

    bool holdActive = false;
    bool detectorActive = false;

    float peakEnvelopeDb = -100.0f;
    float peakReductionDb = 0.0f;
    float currentPeakGain = 1.0f;
    SlidingRms inputDetector;
    SlidingRms outputDetector;
    Telemetry lastTelemetry;
};