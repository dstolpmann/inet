//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <algorithm>
#include "inet/common/ModuleAccess.h"
#include "inet/common/figures/LabeledIconFigure.h"
#include "inet/common/figures/SignalFigure.h"

#ifdef WITH_RADIO
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalTransmission.h"
#include "inet/physicallayer/analogmodel/packetlevel/ScalarReception.h"
#include "inet/physicallayer/analogmodel/packetlevel/ScalarTransmission.h"
#endif // WITH_RADIO

#include "inet/visualizer/physicallayer/MediumCanvasVisualizer.h"

namespace inet {
namespace visualizer {

#ifdef WITH_RADIO

using namespace inet::physicallayer;

Define_Module(MediumCanvasVisualizer);

void MediumCanvasVisualizer::initialize(int stage)
{
    MediumVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        zIndex = par("zIndex");
        const char *signalShapeString = par("signalShape");
        if (!strcmp(signalShapeString, "ring"))
            signalShape = SIGNAL_SHAPE_RING;
        else if (!strcmp(signalShapeString, "sphere"))
            signalShape = SIGNAL_SHAPE_SPHERE;
        else
            throw cRuntimeError("Unknown signalShape parameter value: '%s'", signalShapeString);
        signalOpacity = par("signalOpacity");
        signalRingCount = par("signalRingCount");
        signalRingSize = par("signalRingSize");
        signalFadingDistance = par("signalFadingDistance");
        signalFadingFactor = par("signalFadingFactor");
        signalWaveCount = par("signalWaveCount");
        signalWaveLength = par("signalWaveLength");
        signalWaveWidth = par("signalWaveWidth");
        signalWaveFadingAnimationSpeedFactor = par("signalWaveFadingAnimationSpeedFactor");
        cCanvas *canvas = visualizationTargetModule->getCanvas();
        if (displaySignals) {
            signalLayer = new cGroupFigure("communication");
            signalLayer->setZIndex(zIndex);
            signalLayer->insertBelow(canvas->getSubmodulesLayer());
        }
        displayCommunicationHeat = par("displayCommunicationHeat");
        if (displayCommunicationHeat) {
            communicationHeat = new HeatMapFigure(communicationHeatMapSize, "communication heat");
            communicationHeat->setZIndex(zIndex);
            communicationHeat->setTags((std::string("successful_reception heat ") + tags).c_str());
            canvas->addFigure(communicationHeat, 0);
        }
        animationSpeedInterpolator.setCurrentAnimationSpeed(0);
        animationSpeedInterpolator.setTargetAnimationSpeed(AnimationPosition::REAL_TIME, 0, 0);
        networkNodeVisualizer = getModuleFromPar<NetworkNodeCanvasVisualizer>(par("networkNodeVisualizerModule"), this);
    }
    else if (stage == INITSTAGE_LAST) {
        canvasProjection = CanvasProjection::getCanvasProjection(visualizationTargetModule->getCanvas());
        if (communicationHeat != nullptr) {
            const IMediumLimitCache *mediumLimitCache = radioMedium->getMediumLimitCache();
            Coord min = mediumLimitCache->getMinConstraintArea();
            Coord max = mediumLimitCache->getMaxConstraintArea();
            cFigure::Point o = canvasProjection->computeCanvasPoint(Coord::ZERO);
            cFigure::Point x = canvasProjection->computeCanvasPoint(Coord(1, 0, 0));
            cFigure::Point y = canvasProjection->computeCanvasPoint(Coord(0, 1, 0));
            double t1 = o.x;
            double t2 = o.y;
            double a = x.x - t1;
            double b = x.y - t2;
            double c = y.x - t1;
            double d = y.y - t2;
            communicationHeat->setTransform(cFigure::Transform(a, b, c, d, t1, t2));
            communicationHeat->setPosition(cFigure::Point((min.x + max.x) / 2, (min.y + max.y) / 2));
            communicationHeat->setWidth(max.x - min.x);
            communicationHeat->setHeight(max.y - min.y);
        }
        if (displayMainPowerDensityMap) {
            mainPowerDensityMapFigure = new HeatMapPlotFigure();
            mainPowerDensityMapFigure->setTags("signal_main_power_density_map");
            mainPowerDensityMapFigure->setTooltip("This plot represents the signal power density over space");
            mainPowerDensityMapFigure->setZIndex(zIndex);
            mainPowerDensityMapFigure->setXAxisLabel("[m]");
            mainPowerDensityMapFigure->setYAxisLabel("[m]");
            mainPowerDensityMapFigure->setXValueFormat("%.3g");
            mainPowerDensityMapFigure->setYValueFormat("%.3g");
            const auto& displayString = visualizationTargetModule->getDisplayString();
            auto width = atof(displayString.getTagArg("bgb", 0));
            auto height = atof(displayString.getTagArg("bgb", 1));
            auto minPosition = canvasProjection->computeCanvasPointInverse(cFigure::Point(0, 0), 0);
            auto maxPosition = canvasProjection->computeCanvasPointInverse(cFigure::Point(width, height), 0);
            mainPowerDensityMapFigure->setMinX(minPosition.x);
            mainPowerDensityMapFigure->setMaxX(maxPosition.x);
            mainPowerDensityMapFigure->setMinY(minPosition.y);
            mainPowerDensityMapFigure->setMaxY(maxPosition.y);
            mainPowerDensityMapFigure->setPlotSize(cFigure::Point(width, height), cFigure::Point(width / powerDensityMapFigureWidth * powerDensityMapPixmapWidth, height / powerDensityMapFigureHeight * powerDensityMapPixmapHeight));
            cCanvas *canvas = visualizationTargetModule->getCanvas();
            canvas->addFigure(mainPowerDensityMapFigure, 0);
        }
        for (cModule::SubmoduleIterator it(visualizationSubjectModule); !it.end(); it++) {
            auto networkNode = *it;
            if (isNetworkNode(networkNode) && networkNodeFilter.matches(networkNode)) {
                auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(networkNode);
                if (displayPowerDensityMaps) {
                    auto powerDensityMapFigure = new HeatMapPlotFigure();
                    powerDensityMapFigure->setTags("signal_power_density_map");
                    powerDensityMapFigure->setTooltip("This plot represents the signal power density over space");
                    powerDensityMapFigure->setZIndex(zIndex);
                    powerDensityMapFigure->setXAxisLabel("[m]");
                    powerDensityMapFigure->setYAxisLabel("[m]");
                    powerDensityMapFigure->setXValueFormat("%.3g");
                    powerDensityMapFigure->setYValueFormat("%.3g");
                    powerDensityMapFigure->setPlotSize(cFigure::Point(powerDensityMapFigureWidth, powerDensityMapFigureHeight), cFigure::Point(powerDensityMapPixmapWidth, powerDensityMapPixmapHeight));
                    // TODO: center on node to align in space coordinates
                    powerDensityMapFigure->refreshDisplay();
                    networkNodeVisualization->addAnnotation(powerDensityMapFigure, powerDensityMapFigure->getSize(), spectogramPlacementHint, spectogramPlacementPriority);
                    powerDensityMapFigures[networkNode] = powerDensityMapFigure;
                }
                if (displaySpectrums) {
                    auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(networkNode);
                    auto spectrumFigure = new PlotFigure();
                    spectrumFigure->setTags("signal_spectrum");
                    spectrumFigure->setTooltip("This plot represents the signal spectral power density");
                    spectrumFigure->setZIndex(zIndex);
                    spectrumFigure->setNumSeries(3);
                    spectrumFigure->setLineColor(0, cFigure::parseColor("darkred"));
                    spectrumFigure->setLineColor(1, cFigure::parseColor("darkgreen"));
                    spectrumFigure->setLineColor(2, cFigure::parseColor("darkblue"));
                    spectrumFigure->setXAxisLabel("[GHz]");
                    spectrumFigure->setYAxisLabel("[dBmW/MHz]");
                    spectrumFigure->setXValueFormat("%.3g");
                    spectrumFigure->setYValueFormat("%.3g");
                    spectrumFigure->setPlotSize(cFigure::Point(spectrumFigureWidth, spectrumFigureHeight));
                    spectrumFigure->refreshDisplay();
                    networkNodeVisualization->addAnnotation(spectrumFigure, spectrumFigure->getSize(), spectrumPlacementHint, spectrumPlacementPriority);
                    spectrumFigures[networkNode] = spectrumFigure;
                }
                if (displaySpectograms) {
                    auto spectogramFigure = new HeatMapPlotFigure();
                    spectogramFigure->setTags("signal_spectogram");
                    spectogramFigure->setTooltip("This plot represents the signal power density over time and frequency");
                    spectogramFigure->setZIndex(zIndex);
                    spectogramFigure->setXAxisLabel("[GHz]");
                    spectogramFigure->setYAxisLabel("[s]");
                    spectogramFigure->setXValueFormat("%.3g");
                    spectogramFigure->setYValueFormat("%.3g");
                    spectogramFigure->setPlotSize(cFigure::Point(spectogramFigureWidth, spectogramFigureHeight), cFigure::Point(spectogramPixmapWidth, spectogramPixmapHeight));
                    spectogramFigure->refreshDisplay();
                    networkNodeVisualization->addAnnotation(spectogramFigure, spectogramFigure->getSize(), spectogramPlacementHint, spectogramPlacementPriority);
                    spectrogramFigures[networkNode] = spectogramFigure;
                }
            }
        }
    }
}

void MediumCanvasVisualizer::refreshDisplay() const
{
    if (displaySignals || displayMainPowerDensityMap || displayPowerDensityMaps)
        const_cast<MediumCanvasVisualizer *>(this)->setAnimationSpeed();
    if (displaySignals) {
        for (auto transmission : transmissions)
            if (matchesTransmission(transmission))
                refreshSignalFigure(transmission);
    }
    if (!getEnvir()->isExpressMode()) {
        if (displayMainPowerDensityMap)
            refreshMainPowerDensityMapFigure();
        if (displayPowerDensityMaps) {
            for (auto it : powerDensityMapFigures)
                refreshPowerDensityMapFigure(it.first, it.second);
        }
        if (displaySpectrums)
            for (auto it : spectrumFigures)
                refreshSpectrumFigure(it.first, it.second);
        if (displaySpectograms)
            for (auto it : spectrogramFigures)
                refreshSpectogramFigure(it.first, it.second);
        if (displayCommunicationHeat)
            communicationHeat->coolDown();
    }
}

void MediumCanvasVisualizer::refreshMainPowerDensityMapFigure() const
{
    if (signalMinFrequency < signalMaxFrequency) {
        mainPowerDensityMapFigure->setXTickCount(powerDensityMapFigureXTickCount);
        mainPowerDensityMapFigure->setYTickCount(powerDensityMapFigureYTickCount);
        mainPowerDensityMapFigure->clearValues();
        refreshPowerDensityMapFigurePowerFunction(mediumPowerDensityFunction, mainPowerDensityMapFigure, 2);
        mainPowerDensityMapFigure->bakeValues();
        mainPowerDensityMapFigure->refreshDisplay();
    }
}

void MediumCanvasVisualizer::refreshPowerDensityMapFigure(const cModule *networkNode, HeatMapPlotFigure *figure) const
{
    if (signalMinFrequency < signalMaxFrequency) {
        const ITransmission *transmissionInProgress;
        const ITransmission *receptionInProgress;
        const IAntenna *antenna;
        IMobility *mobility;
        std::tie(transmissionInProgress, receptionInProgress, antenna, mobility) = extractSpectrumFigureParameters(networkNode);
        auto position = antenna->getMobility()->getCurrentPosition();
        auto point = canvasProjection->computeCanvasPoint(position);
        auto minPosition = canvasProjection->computeCanvasPointInverse(cFigure::Point(point.x - powerDensityMapFigureWidth / 2, point.y - powerDensityMapFigureHeight / 2), 0);
        auto maxPosition = canvasProjection->computeCanvasPointInverse(cFigure::Point(point.x + powerDensityMapFigureWidth / 2, point.y + powerDensityMapFigureHeight / 2), 0);
        figure->setMinX(minPosition.x);
        figure->setMaxX(maxPosition.x);
        figure->setMinY(minPosition.y);
        figure->setMaxY(maxPosition.y);
        figure->setXTickCount(powerDensityMapFigureXTickCount);
        figure->setYTickCount(powerDensityMapFigureYTickCount);
        figure->clearValues();
        if (!strcmp(powerDensityMapMode, "total"))
            refreshPowerDensityMapFigurePowerFunction(mediumPowerDensityFunction, figure, 2);
        else if (!strcmp(powerDensityMapMode, "signal")) {
            if (transmissionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(transmissionInProgress)->second;
                refreshPowerDensityMapFigurePowerFunction(signalPowerDensityFunction, figure, 1);
            }
            else if (receptionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(receptionInProgress)->second;
                refreshPowerDensityMapFigurePowerFunction(signalPowerDensityFunction, figure, 1);
            }
        }
        else if (!strcmp(powerDensityMapMode, "auto")) {
            if (transmissionInProgress == nullptr && receptionInProgress == nullptr)
                refreshPowerDensityMapFigurePowerFunction(mediumPowerDensityFunction, figure, 2);
            else if (transmissionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(transmissionInProgress)->second;
                const auto& noisePowerDensityFunction = noisePowerDensityFunctions.find(transmissionInProgress)->second;
                refreshPowerDensityMapFigurePowerFunction(noisePowerDensityFunction, figure, 0);
                refreshPowerDensityMapFigurePowerFunction(signalPowerDensityFunction, figure, 1);
            }
            else if (receptionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(receptionInProgress)->second;
                const auto& noisePowerDensityFunction = noisePowerDensityFunctions.find(receptionInProgress)->second;
                refreshPowerDensityMapFigurePowerFunction(noisePowerDensityFunction, figure, 0);
                refreshPowerDensityMapFigurePowerFunction(signalPowerDensityFunction, figure, 1);
            }
        }
        else
            throw cRuntimeError("Unknown signalFigureMode");
        figure->bakeValues();
        figure->refreshDisplay();
    }
}

void MediumCanvasVisualizer::refreshPowerDensityMapFigurePowerFunction(const Ptr<const math::IFunction<WpHz, math::Domain<m, m, m, simsec, Hz>>>& powerDensityFunction, HeatMapPlotFigure *figure, int channel) const
{
    Coord minPosition(figure->getMinX(), figure->getMinY(), 0);
    Coord maxPosition(figure->getMaxX(), figure->getMaxY(), 0);
    Ptr<const math::IFunction<W, math::Domain<m, m, m, simsec>>> powerFunction;
    if (powerDensityMapBandwidth == Hz(0)) {
        powerFunction = nullptr;
        figure->setMinValue(wpHz2dBmWpMHz(WpHz(signalMinPowerDensity).get()));
        figure->setMaxValue(wpHz2dBmWpMHz(WpHz(signalMaxPowerDensity).get()));
    }
    else if (std::isinf(powerDensityMapBandwidth.get())) {
        powerFunction = integrate<WpHz, Domain<m, m, m, simsec, Hz>, 0b11110, W, Domain<m, m, m, simsec>>(powerDensityFunction);
        figure->setMinValue(mW2dBmW(mW(signalMinPower).get()));
        figure->setMaxValue(mW2dBmW(mW(signalMaxPower).get()));
        throw cRuntimeError("TODO");
    }
    else {
        // TODO:
//            auto bandpassFilterFunction = makeShared<OneDimensionalBoxcarFunction<double, Hz>>(powerDensityMapCenterFrequency - powerDensityMapBandwidth / 2, powerDensityMapCenterFrequency + powerDensityMapBandwidth / 2, 1);
//            powerFunction = integrate<WpHz, Domain<m, m, m, simsec, Hz>, 0b11110, W, Domain<m, m, m, simsec>>(powerDensityFunction->multiply(bandpassFilter));
        figure->setMinValue(mW2dBmW(mW(signalMinPower).get()));
        figure->setMaxValue(mW2dBmW(mW(signalMaxPower).get()));
        throw cRuntimeError("TODO");
    }
    auto plotSize = figure->getPlotSize();
    auto pixmapSize = figure->getPixmapSize();
    if (powerDensityMapSampling) {
        for (int x = 0; x < pixmapSize.x; x++) {
            for (int y = 0; y < pixmapSize.y; y++) {
                if (powerFunction == nullptr) {
                    auto value = powerDensityFunction->getValue(Point<m, m, m, simsec, Hz>(m(minPosition.x + x / pixmapSize.x * plotSize.x), m(minPosition.y + y / pixmapSize.y * plotSize.y), m(0), simsec(simTime()), powerDensityMapCenterFrequency));
                    figure->setValue(x, y, wpHz2dBmWpMHz(WpHz(value).get()), channel);
                }
                else {
                    auto value = powerFunction->getValue(Point<m, m, m, simsec>(m(minPosition.x + x / pixmapSize.x * plotSize.x), m(minPosition.y + y / pixmapSize.y * plotSize.y), m(0), simsec(simTime())));
                    figure->setValue(x, y, mW2dBmW(mW(value).get()), channel);
                }
            }
        }
    }
    else {
        Point<m, m, m, simsec, Hz> l(m(minPosition.x), m(minPosition.y), m(minPosition.z), simsec(simTime()), powerDensityMapCenterFrequency);
        Point<m, m, m, simsec, Hz> u(m(maxPosition.x), m(maxPosition.y), m(maxPosition.z), simsec(simTime()), powerDensityMapCenterFrequency);
        Interval<m, m, m, simsec, Hz> i(l, u, 0b00111, 0b00111, 0b00111);
        auto approximatedMediumPowerFunction1 = makeShared<ApproximatedFunction<WpHz, Domain<m, m, m, simsec, Hz>, 0, m>>(m(minPosition.x), m(maxPosition.x), m(maxPosition.x - minPosition.y) / powerDensityMapPixmapWidth * powerDensityMapApproximationSize, &LinearInterpolator<m, WpHz>::singleton, powerDensityFunction);
        auto approximatedMediumPowerFunction2 = makeShared<ApproximatedFunction<WpHz, Domain<m, m, m, simsec, Hz>, 1, m>>(m(minPosition.y), m(maxPosition.y), m(maxPosition.y - minPosition.y) / powerDensityMapPixmapHeight * powerDensityMapApproximationSize, &LinearInterpolator<m, WpHz>::singleton, approximatedMediumPowerFunction1);
        approximatedMediumPowerFunction2->partition(i, [&] (const Interval<m, m, m, simsec, Hz>& j, const IFunction<WpHz, Domain<m, m, m, simsec, Hz>> *partitonPowerFunction) {
            auto lower = j.getLower();
            auto upper = j.getUpper();
            auto x1 = m(std::get<0>(lower)).get();
            auto x2 = m(std::get<0>(upper)).get();
            auto y1 = m(std::get<1>(lower)).get();
            auto y2 = m(std::get<1>(upper)).get();
            if (auto cf = dynamic_cast<const ConstantFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction)) {
                auto v = cf->getConstantValue() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(cf->getConstantValue()).get());
                figure->setConstantValue(x1, x2, y1, y2, v, channel);
            }
            else if (auto lf = dynamic_cast<const UnilinearFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction)) {
                auto vl = lf->getRLower() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(lf->getRLower()).get());
                auto vu = lf->getRUpper() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(lf->getRUpper()).get());
                figure->setLinearValue(x1, x2, y1, y2, vl, vu, lf->getDimension(), channel);
            }
            else if (auto bf = dynamic_cast<const BilinearFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction)) {
                auto v11 = bf->getRLowerLower() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRLowerLower()).get());
                auto v21 = bf->getRLowerUpper() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRLowerUpper()).get());
                auto v12 = bf->getRUpperLower() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRUpperLower()).get());
                auto v22 = bf->getRUpperUpper() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRUpperUpper()).get());
                figure->setBilinearValue(x1, x2, y1, y2, v11, v21, v12, v22, channel);
            }
            else
                throw cRuntimeError("TODO");
        });
    }
}

void MediumCanvasVisualizer::refreshSpectrumFigure(const cModule *networkNode, PlotFigure *figure) const
{
    if (signalMinFrequency < signalMaxFrequency) {
        const ITransmission *transmissionInProgress;
        const ITransmission *receptionInProgress;
        const IAntenna *antenna;
        IMobility *mobility;
        std::tie(transmissionInProgress, receptionInProgress, antenna, mobility) = extractSpectrumFigureParameters(networkNode);
        figure->clearValues(0);
        figure->clearValues(1);
        figure->clearValues(2);
        auto position = mobility->getCurrentPosition();
        if (!strcmp(spectrumMode, "total"))
            refreshSpectrumFigurePowerFunction(mediumPowerDensityFunction, antenna, position, figure, 2);
        else if (!strcmp(spectrumMode, "signal")) {
            if (transmissionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(transmissionInProgress)->second;
                refreshSpectrumFigurePowerFunction(signalPowerDensityFunction, antenna, position, figure, 1);
            }
            else if (receptionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(receptionInProgress)->second;
                refreshSpectrumFigurePowerFunction(signalPowerDensityFunction, antenna, position, figure, 1);
            }
        }
        else if (!strcmp(spectrumMode, "auto")) {
            if (transmissionInProgress == nullptr && receptionInProgress == nullptr)
                refreshSpectrumFigurePowerFunction(mediumPowerDensityFunction, antenna, position, figure, 2);
            else if (transmissionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(transmissionInProgress)->second;
                const auto& noisePowerDensityFunction = noisePowerDensityFunctions.find(transmissionInProgress)->second;
                refreshSpectrumFigurePowerFunction(noisePowerDensityFunction, antenna, position, figure, 0);
                refreshSpectrumFigurePowerFunction(signalPowerDensityFunction, antenna, position, figure, 1);
            }
            else if (receptionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(receptionInProgress)->second;
                const auto& noisePowerDensityFunction = noisePowerDensityFunctions.find(receptionInProgress)->second;
                refreshSpectrumFigurePowerFunction(noisePowerDensityFunction, antenna, position, figure, 0);
                refreshSpectrumFigurePowerFunction(signalPowerDensityFunction, antenna, position, figure, 1);
            }
        }
        else
            throw cRuntimeError("Unknown signalFigureMode");
        double minValue = wpHz2dBmWpMHz(WpHz(signalMinPowerDensity).get());
        double maxValue = wpHz2dBmWpMHz(WpHz(signalMaxPowerDensity).get());
        if (minValue < maxValue) {
            double margin = 0.05 * (maxValue - minValue);
            figure->setMinY(minValue - margin);
            figure->setMaxY(maxValue + margin);
            figure->setXTickCount(spectrumFigureXTickCount);
            figure->setYTickCount(spectrumFigureYTickCount);
        }
        figure->refreshDisplay();
    }
}

void MediumCanvasVisualizer::refreshSpectrumFigurePowerFunction(const Ptr<const IFunction<WpHz, Domain<m, m, m, simsec, Hz>>>& powerFunction, const IAntenna *antenna, const Coord& position, PlotFigure *figure, int series) const
{
    auto marginFrequency = 0.05 * (signalMaxFrequency - signalMinFrequency);
    auto minFrequency = signalMinFrequency - marginFrequency;
    auto maxFrequency = signalMaxFrequency + marginFrequency;
    figure->setMinX(GHz(minFrequency).get());
    figure->setMaxX(GHz(maxFrequency).get());
    Point<m, m, m, simsec, Hz> l(m(position.x), m(position.y), m(position.z), simsec(simTime()), minFrequency);
    Point<m, m, m, simsec, Hz> u(m(position.x), m(position.y), m(position.z), simsec(simTime()), maxFrequency);
    Interval<m, m, m, simsec, Hz> i(l, u, 0b11110, 0b11110, 0b11110);
    auto dx = GHz(maxFrequency - minFrequency).get() * spectrumFigureInterpolationSize / spectrumFigureWidth;
    powerFunction->partition(i, [&] (const Interval<m, m, m, simsec, Hz>& j, const IFunction<WpHz, Domain<m, m, m, simsec, Hz>> *partitonPowerFunction) {
        ASSERT((dynamic_cast<const ConstantFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction) != nullptr || dynamic_cast<const UnilinearFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction) != nullptr));
        auto lower = j.getLower();
        auto upper = j.getUpper();
        // NOTE: the interval is closed at the lower boundary and open at the upper boundary
        //       we want to have the limit of the function's value at the upper boundary from the left
        if (std::get<4>(upper) != std::get<4>(lower))
            std::get<4>(upper) = Hz(std::nextafter(std::get<4>(upper).get(), std::get<4>(lower).get()));
        WpHz power1;
        WpHz power2;
        std::tie(power1, power2) = computePowerForPartitionBounds(powerFunction, lower, upper, partitonPowerFunction, antenna, position);
        // TODO: the function f is assumed to be constant or linear between l1 and u1 but on a logarithmic axis the plot is non-linear
        // TODO: the following interpolation should be part of the PlotFigure along with logarithmic axis support
        auto x1 = GHz(std::get<4>(lower)).get();
        auto x2 = GHz(std::get<4>(upper)).get();
        for (double x = x1; x < x2; x += dx) {
            double xi = x;
            double xj = std::min(x2, x + dx);
            double ai = (xi - x1) / (x2 - x1);
            double aj = (xj - x1) / (x2 - x1);
            auto yi = power1 * (1 - ai) + power2 * ai;
            auto yj = power1 * (1 - aj) + power2 * aj;
            figure->setValue(series, xi, wpHz2dBmWpMHz(WpHz(yi).get()));
            figure->setValue(series, xj, wpHz2dBmWpMHz(WpHz(yj).get()));
        }
    });
}

std::pair<WpHz, WpHz> MediumCanvasVisualizer::computePowerForPartitionBounds(const Ptr<const IFunction<WpHz, Domain<m, m, m, simsec, Hz>>>& powerFunction, const math::Point<m, m, m, simsec, Hz>& lower, const math::Point<m, m, m, simsec, Hz>& upper, const IFunction<WpHz, Domain<m, m, m, simsec, Hz>> *partitonPowerFunction, const IAntenna *antenna, const Coord& position) const
{
    WpHz totalPower1;
    WpHz totalPower2;
    if (antenna != nullptr && antenna->isDirectional()) {
        if (auto sumFunction = dynamicPtrCast<const SumFunction<WpHz, Domain<m, m, m, simsec, Hz>>>(powerFunction)) {
            totalPower1 = WpHz(0);
            totalPower2 = WpHz(0);
            for (auto elementFunction : sumFunction->getElements()) {
                WpHz p1;
                WpHz p2;
                std::tie(p1, p2) = computePowerForDirectionalAntenna(elementFunction, lower, upper, antenna, position);
                totalPower1 += p1;
                totalPower2 += p2;
            }
        }
        else
            std::tie(totalPower1, totalPower2) = computePowerForDirectionalAntenna(powerFunction, lower, upper, antenna, position);
    }
    else {
        totalPower1 = partitonPowerFunction->getValue(lower);
        totalPower2 = partitonPowerFunction->getValue(upper);
    }
    ASSERT(!std::isnan(totalPower1.get()));
    ASSERT(!std::isnan(totalPower2.get()));
    return {totalPower1, totalPower2};
}

std::pair<WpHz, WpHz> MediumCanvasVisualizer::computePowerForDirectionalAntenna(const Ptr<const IFunction<WpHz, Domain<m, m, m, simsec, Hz>>>& powerFunction, const math::Point<m, m, m, simsec, Hz>& lower, const math::Point<m, m, m, simsec, Hz>& upper, const IAntenna *antenna, const Coord& position) const
{
    if (auto rf = dynamicPtrCast<const MultiplicationFunction<WpHz, Domain<m, m, m, simsec, Hz>>>(powerFunction)) {
        if (auto tf = dynamicPtrCast<const PropagatedTransmissionPowerFunction>(rf->getF1())) {
            const Point<m, m, m>& startPosition = tf->getStartPosition();
            double dx = std::get<0>(startPosition).get() - position.x;
            double dy = std::get<1>(startPosition).get() - position.y;
            double dz = std::get<2>(startPosition).get() - position.z;
            double gain = 1;
            if (dx != 0 || dy != 0 || dz != 0) {
                const Quaternion& startOrientation = antenna->getMobility()->getCurrentAngularPosition();
                auto direction = Quaternion::rotationFromTo(Coord::X_AXIS, Coord(dx, dy, dz));
                auto antennaLocalDirection = startOrientation.inverse() * direction;
                gain = antenna->getGain()->computeGain(antennaLocalDirection);
            }
            WpHz power1 = gain * powerFunction->getValue(lower);
            WpHz power2 = gain * powerFunction->getValue(upper);
            ASSERT(!std::isnan(power1.get()));
            ASSERT(!std::isnan(power2.get()));
            return {power1, power2};
        }
    }
    WpHz power1 = powerFunction->getValue(lower);
    WpHz power2 = powerFunction->getValue(upper);
    ASSERT(!std::isnan(power1.get()));
    ASSERT(!std::isnan(power2.get()));
    return {power1, power2};
}

void MediumCanvasVisualizer::refreshSpectogramFigure(const cModule *networkNode, HeatMapPlotFigure *figure) const
{
    if (signalMinFrequency < signalMaxFrequency) {
        const ITransmission *transmissionInProgress;
        const ITransmission *receptionInProgress;
        const IAntenna *antenna;
        IMobility *mobility;
        std::tie(transmissionInProgress, receptionInProgress, antenna, mobility) = extractSpectrumFigureParameters(networkNode);
        double minValue = wpHz2dBmWpMHz(WpHz(signalMinPowerDensity).get());
        double maxValue = wpHz2dBmWpMHz(WpHz(signalMaxPowerDensity).get());
        auto marginFrequency = 0.05 * (signalMaxFrequency - signalMinFrequency);
        auto minFrequency = signalMinFrequency - marginFrequency;
        auto maxFrequency = signalMaxFrequency + marginFrequency;
        figure->setMinX(GHz(minFrequency).get());
        figure->setMaxX(GHz(maxFrequency).get());
        SimTimeUnit signalTimeUnit;
        if (signalMaxTime < 1E-9) {
            signalTimeUnit = SIMTIME_PS;
            figure->setYAxisLabel("[ps]");
        }
        else if (signalMaxTime < 1E-6) {
            signalTimeUnit = SIMTIME_NS;
            figure->setYAxisLabel("[ns]");
        }
        else if (signalMaxTime < 1E-3) {
            signalTimeUnit = SIMTIME_US;
            figure->setYAxisLabel("[us]");
        }
        else if (signalMaxTime < 1) {
            signalTimeUnit = SIMTIME_MS;
            figure->setYAxisLabel("[ms]");
        }
        else {
            signalTimeUnit = SIMTIME_S;
            figure->setYAxisLabel("[s]");
        }
        figure->setMinY(-signalMaxTime.dbl() / std::pow(10, signalTimeUnit));
        figure->setMaxY(signalMaxTime.dbl() / std::pow(10, signalTimeUnit));
        figure->setMinValue(minValue);
        figure->setMaxValue(maxValue);
        figure->clearValues();
        auto position = mobility->getCurrentPosition();
        if (!strcmp(spectogramMode, "total"))
            refreshSpectogramFigurePowerFunction(mediumPowerDensityFunction, position, signalTimeUnit, figure, 2);
        else if (!strcmp(spectogramMode, "signal")) {
            if (transmissionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(transmissionInProgress)->second;
                refreshSpectogramFigurePowerFunction(signalPowerDensityFunction, position, signalTimeUnit, figure, 1);
            }
            else if (receptionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(receptionInProgress)->second;
                refreshSpectogramFigurePowerFunction(signalPowerDensityFunction, position, signalTimeUnit, figure, 1);
            }
        }
        else if (!strcmp(spectogramMode, "auto")) {
            if (transmissionInProgress == nullptr && receptionInProgress == nullptr)
                refreshSpectogramFigurePowerFunction(mediumPowerDensityFunction, position, signalTimeUnit, figure, 2);
            else if (transmissionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(transmissionInProgress)->second;
                const auto& noisePowerDensityFunction = noisePowerDensityFunctions.find(transmissionInProgress)->second;
                refreshSpectogramFigurePowerFunction(noisePowerDensityFunction, position, signalTimeUnit, figure, 0);
                refreshSpectogramFigurePowerFunction(signalPowerDensityFunction, position, signalTimeUnit, figure, 1);
            }
            else if (receptionInProgress != nullptr) {
                const auto& signalPowerDensityFunction = signalPowerDensityFunctions.find(receptionInProgress)->second;
                const auto& noisePowerDensityFunction = noisePowerDensityFunctions.find(receptionInProgress)->second;
                refreshSpectogramFigurePowerFunction(noisePowerDensityFunction, position, signalTimeUnit, figure, 0);
                refreshSpectogramFigurePowerFunction(signalPowerDensityFunction, position, signalTimeUnit, figure, 1);
            }
        }
        else
            throw cRuntimeError("Unknown signalFigureMode");
        figure->setXTickCount(spectogramFigureXTickCount);
        figure->setYTickCount(spectogramFigureYTickCount);
        figure->bakeValues();
        figure->refreshDisplay();
    }
}

void MediumCanvasVisualizer::refreshSpectogramFigurePowerFunction(const Ptr<const IFunction<WpHz, Domain<m, m, m, simsec, Hz>>>& powerFunction, const Coord& position, SimTimeUnit signalTimeUnit, HeatMapPlotFigure *figure, int channel) const
{
    auto minTime = simTime() - signalMaxTime;
    auto maxTime = simTime() + signalMaxTime;
    auto marginFrequency = 0.05 * (signalMaxFrequency - signalMinFrequency);
    auto minFrequency = signalMinFrequency - marginFrequency;
    auto maxFrequency = signalMaxFrequency + marginFrequency;
    Point<m, m, m, simsec, Hz> l(m(position.x), m(position.y), m(position.z), simsec(minTime), minFrequency);
    Point<m, m, m, simsec, Hz> u(m(position.x), m(position.y), m(position.z), simsec(maxTime), maxFrequency);
    Interval<m, m, m, simsec, Hz> i(l, u, 0b11100, 0b11100, 0b11100);
    powerFunction->partition(i, [&] (const Interval<m, m, m, simsec, Hz>& j, const IFunction<WpHz, Domain<m, m, m, simsec, Hz>> *partitonPowerFunction) {
        auto lower = j.getLower();
        auto upper = j.getUpper();
        auto x1 = GHz(std::get<4>(lower)).get();
        auto x2 = GHz(std::get<4>(upper)).get();
        auto y1 = (simsec(std::get<3>(lower)).get() - simTime()).dbl() / std::pow(10, signalTimeUnit);
        auto y2 = (simsec(std::get<3>(upper)).get() - simTime()).dbl() / std::pow(10, signalTimeUnit);
        if (auto cf = dynamic_cast<const ConstantFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction)) {
            auto v = cf->getConstantValue() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(cf->getConstantValue()).get());
            figure->setConstantValue(x1, x2, y1, y2, v, channel);
        }
        else if (auto lf = dynamic_cast<const UnilinearFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction)) {
            auto vl = lf->getRLower() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(lf->getRLower()).get());
            auto vu = lf->getRUpper() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(lf->getRUpper()).get());
            figure->setLinearValue(x1, x2, y1, y2, vl, vu, 4 - lf->getDimension(), channel);
        }
        else if (auto bf = dynamic_cast<const BilinearFunction<WpHz, Domain<m, m, m, simsec, Hz>> *>(partitonPowerFunction)) {
            auto v11 = bf->getRLowerLower() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRLowerLower()).get());
            auto v21 = bf->getRLowerUpper() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRLowerUpper()).get());
            auto v12 = bf->getRUpperLower() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRUpperLower()).get());
            auto v22 = bf->getRUpperUpper() == WpHz(0) ? figure->getMinValue() : wpHz2dBmWpMHz(WpHz(bf->getRUpperUpper()).get());
            figure->setBilinearValue(x1, x2, y1, y2, v11, v21, v12, v22, 2);
        }
        else
            throw cRuntimeError("TODO");
    });
}

std::tuple<const ITransmission *, const ITransmission *, const IAntenna *, IMobility *> MediumCanvasVisualizer::extractSpectrumFigureParameters(const cModule *networkNode) const
{
    // TODO: what about multiple radios? what if it's not called wlan, this is quite accidental, etc.?
    const IRadio *radio = dynamic_cast<const IRadio *>(networkNode);
    if (radio == nullptr) {
        auto wlan0 = networkNode->getSubmodule("wlan", 0);
        if (wlan0 != nullptr)
            radio = dynamic_cast<IRadio *>(wlan0->getSubmodule("radio"));
    }
    auto transmissionInProgress = radio != nullptr ? radio->getTransmissionInProgress() : nullptr;
    auto receptionInProgress = radio != nullptr ? radio->getReceptionInProgress() : nullptr;
    const IAntenna *antenna = nullptr;
    if (radio != nullptr)
        antenna = radio->getAntenna();
    if (antenna == nullptr)
        antenna = dynamic_cast<IAntenna *>(networkNode->getSubmodule("antenna"));
    IMobility *mobility = nullptr;
    if (antenna != nullptr)
        mobility = antenna->getMobility();
    else
        mobility = check_and_cast<IMobility *>(networkNode->getSubmodule("mobility"));
    return std::tuple<const ITransmission *, const ITransmission *, const IAntenna *, IMobility *>{transmissionInProgress, receptionInProgress, antenna, mobility};
}

void MediumCanvasVisualizer::setAnimationSpeed()
{
    SignalInProgress newSignalInProgress = SIP_NONE;
    double newSignalTransmissionAnimationSpeed = DBL_MAX;
    for (auto transmission : transmissions) {
        if (matchesTransmission(transmission)) {
            if (isSignalPropagationInProgress(transmission))
                newSignalInProgress = SIP_PROPAGATION;
            if (isSignalTransmissionInProgress(transmission)) {
                if (newSignalInProgress == SIP_NONE)
                    newSignalInProgress = SIP_TRANSMISSION;
                // TODO: overwrite only...
                if (std::isnan(signalTransmissionAnimationSpeed))
                    newSignalTransmissionAnimationSpeed = std::min(newSignalTransmissionAnimationSpeed, transmission->getDuration().dbl() / signalTransmissionAnimationTime);
            }
        }
    }
    if (newSignalTransmissionAnimationSpeed != DBL_MAX)
        defaultSignalTransmissionAnimationSpeed = newSignalTransmissionAnimationSpeed;
    double currentSignalPropagationAnimationSpeed = std::isnan(signalPropagationAnimationSpeed) ? defaultSignalPropagationAnimationSpeed : signalPropagationAnimationSpeed;
    double currentSignalTransmissionAnimationSpeed = std::isnan(signalTransmissionAnimationSpeed) ? defaultSignalTransmissionAnimationSpeed : signalTransmissionAnimationSpeed;
    AnimationPosition currentPosition;
    if (lastSignalInProgress == SIP_NONE) {
        if (newSignalInProgress == SIP_NONE) {
            if (animationSpeedInterpolator.getCurrentAnimationSpeed() == animationSpeedInterpolator.getTargetAnimationSpeed())
                animationSpeedInterpolator.setAnimationSpeed(0);
        }
        else if (newSignalInProgress == SIP_PROPAGATION)
            animationSpeedInterpolator.setAnimationSpeed(currentSignalPropagationAnimationSpeed);
        else if (newSignalInProgress == SIP_TRANSMISSION)
            animationSpeedInterpolator.setAnimationSpeed(currentSignalTransmissionAnimationSpeed);
    }
    else if (lastSignalInProgress == SIP_PROPAGATION) {
        if (newSignalInProgress == SIP_NONE) {
            animationSpeedInterpolator.setCurrentAnimationSpeed(currentSignalPropagationAnimationSpeed);
            animationSpeedInterpolator.setTargetAnimationSpeed(AnimationPosition::REAL_TIME, currentPosition.getRealTime() + signalAnimationSpeedChangeTime, currentSignalTransmissionAnimationSpeed);
        }
        else if (newSignalInProgress == SIP_PROPAGATION)
            ; // void
        else if (newSignalInProgress == SIP_TRANSMISSION) {
            animationSpeedInterpolator.setCurrentAnimationSpeed(currentSignalPropagationAnimationSpeed);
            animationSpeedInterpolator.setTargetAnimationSpeed(AnimationPosition::REAL_TIME, currentPosition.getRealTime() + signalAnimationSpeedChangeTime, currentSignalTransmissionAnimationSpeed);
        }
    }
    else if (lastSignalInProgress == SIP_TRANSMISSION) {
        if (newSignalInProgress == SIP_NONE)
            animationSpeedInterpolator.setAnimationSpeed(0);
        else if (newSignalInProgress == SIP_PROPAGATION)
            animationSpeedInterpolator.setAnimationSpeed(currentSignalPropagationAnimationSpeed);
        else if (newSignalInProgress == SIP_TRANSMISSION)
            ; // void
    }
    lastSignalInProgress = newSignalInProgress;
    double animationSpeed = animationSpeedInterpolator.getCurrentAnimationSpeed();
    ASSERT(!std::isnan(animationSpeed));
    visualizationTargetModule->getCanvas()->setAnimationSpeed(animationSpeed, this);
}

cFigure *MediumCanvasVisualizer::getSignalDepartureFigure(const IRadio *radio) const
{
    auto it = signalDepartureFigures.find(radio);
    if (it == signalDepartureFigures.end())
        return nullptr;
    else
        return it->second;
}

void MediumCanvasVisualizer::setSignalDepartureFigure(const IRadio *radio, cFigure *figure)
{
    signalDepartureFigures[radio] = figure;
}

cFigure *MediumCanvasVisualizer::removeSignalDepartureFigure(const IRadio *radio)
{
    auto it = signalDepartureFigures.find(radio);
    if (it == signalDepartureFigures.end())
        return nullptr;
    else {
        signalDepartureFigures.erase(it);
        return it->second;
    }
}

cFigure *MediumCanvasVisualizer::getSignalArrivalFigure(const IRadio *radio) const
{
    auto it = signalArrivalFigures.find(radio);
    if (it == signalArrivalFigures.end())
        return nullptr;
    else
        return it->second;
}

void MediumCanvasVisualizer::setSignalArrivalFigure(const IRadio *radio, cFigure *figure)
{
    signalArrivalFigures[radio] = figure;
}

cFigure *MediumCanvasVisualizer::removeSignalArrivalFigure(const IRadio *radio)
{
    auto it = signalArrivalFigures.find(radio);
    if (it == signalArrivalFigures.end())
        return nullptr;
    else {
        signalArrivalFigures.erase(it);
        return it->second;
    }
}

cFigure *MediumCanvasVisualizer::getSignalFigure(const ITransmission *transmission) const
{
    auto it = signalFigures.find(transmission);
    if (it == signalFigures.end())
        return nullptr;
    else
        return it->second;
}

void MediumCanvasVisualizer::setSignalFigure(const ITransmission *transmission, cFigure *figure)
{
    signalFigures[transmission] = figure;
}

cFigure *MediumCanvasVisualizer::removeSignalFigure(const ITransmission *transmission)
{
    auto it = signalFigures.find(transmission);
    if (it == signalFigures.end())
        return nullptr;
    else {
        signalFigures.erase(it);
        return it->second;
    }
}

cGroupFigure* MediumCanvasVisualizer::createSignalFigure(const ITransmission* transmission) const
{
    cFigure::Point position = canvasProjection->computeCanvasPoint(transmission->getStartPosition());
    cGroupFigure* groupFigure = new cGroupFigure("signal");
    cFigure::Color color = signalColorSet.getColor(transmission->getId());
    SignalFigure* signalFigure = new SignalFigure("bubble");
    signalFigure->setTags((std::string("propagating_signal ") + tags).c_str());
    signalFigure->setTooltip("These rings represents a signal propagating through the medium");
    signalFigure->setAssociatedObject(const_cast<cObject *>(check_and_cast<const cObject *>(transmission)));
    signalFigure->setRingCount(signalRingCount);
    signalFigure->setRingSize(signalRingSize);
    signalFigure->setFadingDistance(signalFadingDistance);
    signalFigure->setFadingFactor(signalFadingFactor);
    signalFigure->setWaveCount(signalWaveCount);
    signalFigure->setWaveLength(signalWaveLength);
    signalFigure->setWaveWidth(signalWaveWidth);
    signalFigure->setOpacity(signalOpacity);
    signalFigure->setColor(color);
    signalFigure->setBounds(cFigure::Rectangle(position.x, position.y, 0, 0));
    signalFigure->refresh();
    groupFigure->addFigure(signalFigure);
    cLabelFigure* nameFigure = new cLabelFigure("packet name");
    nameFigure->setPosition(position);
    nameFigure->setTags((std::string("propagating_signal packet_name label ") + tags).c_str());
    auto packet = transmission->getPacket();
    if (packet != nullptr)
        nameFigure->setText(packet->getName());
    nameFigure->setColor(color);
    groupFigure->addFigure(nameFigure);
    return groupFigure;
}

void MediumCanvasVisualizer::refreshSignalFigure(const ITransmission *transmission) const
{
    const IPropagation *propagation = radioMedium->getPropagation();
    cFigure *groupFigure = getSignalFigure(transmission);
    double startRadius = propagation->getPropagationSpeed().get() * (simTime() - transmission->getStartTime()).dbl();
    double endRadius = std::max(0.0, propagation->getPropagationSpeed().get() * (simTime() - transmission->getEndTime()).dbl());
    if (groupFigure) {
        SignalFigure *signalFigure = static_cast<SignalFigure *>(groupFigure->getFigure(0));
        cLabelFigure *labelFigure = static_cast<cLabelFigure *>(groupFigure->getFigure(1));
        double phi = transmission->getId();
        labelFigure->setTransform(cFigure::Transform().translate(endRadius * sin(phi), endRadius * cos(phi)));
        const Coord transmissionStart = transmission->getStartPosition();
        // KLUDGE: to workaround overflow bugs in drawing, Tkenv?
        double offset = std::fmod(startRadius, signalFigure->getWaveLength());
//        if (startRadius > 10000)
//            startRadius = 10000;
//        if (endRadius > 10000)
//            endRadius = 10000;
        switch (signalShape) {
            case SIGNAL_SHAPE_RING: {
                // determine the rotated 2D canvas points by computing the 2D affine transformation from the 3D transformation of the environment
                cFigure::Point o = canvasProjection->computeCanvasPoint(transmissionStart);
                cFigure::Point x = canvasProjection->computeCanvasPoint(transmissionStart + Coord(1, 0, 0));
                cFigure::Point y = canvasProjection->computeCanvasPoint(transmissionStart + Coord(0, 1, 0));
                double t1 = o.x;
                double t2 = o.y;
                double a = x.x - t1;
                double b = x.y - t2;
                double c = y.x - t1;
                double d = y.y - t2;
                signalFigure->setTransform(cFigure::Transform(a, b, c, d, t1, t2));
                signalFigure->setBounds(cFigure::Rectangle(-startRadius, -startRadius, startRadius * 2, startRadius * 2));
                signalFigure->setInnerRx(endRadius);
                signalFigure->setInnerRy(endRadius);
                signalFigure->setWaveOffset(offset);
                double currentSignalPropagationAnimationSpeed = std::isnan(signalPropagationAnimationSpeed) ? defaultSignalPropagationAnimationSpeed : signalPropagationAnimationSpeed;
                signalFigure->setWaveOpacityFactor(std::min(1.0, currentSignalPropagationAnimationSpeed / getSimulation()->getEnvir()->getAnimationSpeed() / signalWaveFadingAnimationSpeedFactor));
                signalFigure->refresh();
                break;
            }
            case SIGNAL_SHAPE_SPHERE: {
                // a sphere looks like a circle from any view angle
                cFigure::Point center = canvasProjection->computeCanvasPoint(transmissionStart);
                signalFigure->setBounds(cFigure::Rectangle(center.x - startRadius, center.y - startRadius, 2 * startRadius, 2 * startRadius));
                signalFigure->setInnerRx(endRadius);
                signalFigure->setInnerRy(endRadius);
                break;
            }
            default:
                throw cRuntimeError("Unimplemented signal shape");
        }
    }
}

void MediumCanvasVisualizer::handleRadioAdded(const IRadio *radio)
{
    Enter_Method_Silent();
    auto module = check_and_cast<const cModule *>(radio);
    auto networkNode = getContainingNode(module);
    if (networkNodeFilter.matches(networkNode)) {
        auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(networkNode);
        if (networkNodeVisualization == nullptr)
            throw cRuntimeError("Cannot create medium visualization for '%s', because network node visualization is not found for '%s'", module->getFullPath().c_str(), networkNode->getFullPath().c_str());
        if (displayInterferenceRanges || (module->hasPar("displayInterferenceRange") && module->par("displayInterferenceRange"))) {
            auto interferenceRangeFigure = new cOvalFigure("interferenceRange");
            m maxInterferenceRange = check_and_cast<const IRadioMedium *>(radio->getMedium())->getMediumLimitCache()->getMaxInterferenceRange(radio);
            interferenceRangeFigure->setTags((std::string("interference_range ") + tags).c_str());
            interferenceRangeFigure->setTooltip("This circle represents the interference range of a wireless interface");
            interferenceRangeFigure->setBounds(cFigure::Rectangle(-maxInterferenceRange.get(), -maxInterferenceRange.get(), 2 * maxInterferenceRange.get(), 2 * maxInterferenceRange.get()));
            interferenceRangeFigure->setLineColor(interferenceRangeLineColor);
            interferenceRangeFigure->setLineStyle(interferenceRangeLineStyle);
            interferenceRangeFigure->setLineWidth(interferenceRangeLineWidth);
            networkNodeVisualization->addFigure(interferenceRangeFigure);
        }
        if (displayCommunicationRanges || (module->hasPar("displayCommunicationRange") && module->par("displayCommunicationRange"))) {
            auto communicationRangeFigure = new cOvalFigure("communicationRange");
            m maxCommunicationRange = check_and_cast<const IRadioMedium *>(radio->getMedium())->getMediumLimitCache()->getMaxCommunicationRange(radio);
            communicationRangeFigure->setTags((std::string("communication_range ") + tags).c_str());
            communicationRangeFigure->setTooltip("This circle represents the communication range of a wireless interface");
            communicationRangeFigure->setBounds(cFigure::Rectangle(-maxCommunicationRange.get(), -maxCommunicationRange.get(), 2 * maxCommunicationRange.get(), 2 * maxCommunicationRange.get()));
            communicationRangeFigure->setLineColor(communicationRangeLineColor);
            communicationRangeFigure->setLineStyle(communicationRangeLineStyle);
            communicationRangeFigure->setLineWidth(communicationRangeLineWidth);
            networkNodeVisualization->addFigure(communicationRangeFigure);
        }
        if (displaySignalDepartures || displaySignalArrivals) {
            if (displaySignalDepartures) {
                std::string imageName = par("signalDepartureImage");
                auto signalDepartureFigure = new LabeledIconFigure("signalDeparture");
                signalDepartureFigure->setTags((std::string("signal_departure ") + tags).c_str());
                signalDepartureFigure->setTooltip("This icon represents an ongoing signal departure");
                signalDepartureFigure->setVisible(false);
                auto iconFigure = signalDepartureFigure->getIconFigure();
                iconFigure->setImageName(imageName.substr(0, imageName.find_first_of(".")).c_str());
                iconFigure->setAnchor(cFigure::ANCHOR_NW);
                auto labelFigure = signalDepartureFigure->getLabelFigure();
                labelFigure->setPosition(iconFigure->getBounds().getSize() / 2);
                networkNodeVisualization->addAnnotation(signalDepartureFigure, signalDepartureFigure->getBounds().getSize(), signalDeparturePlacementHint, signalDeparturePlacementPriority);
                setSignalDepartureFigure(radio, signalDepartureFigure);
            }
            if (displaySignalArrivals) {
                std::string imageName = par("signalArrivalImage");
                auto signalArrivalFigure = new LabeledIconFigure("signalArrival");
                signalArrivalFigure->setTags((std::string("signal_arrival ") + tags).c_str());
                signalArrivalFigure->setTooltip("This icon represents an ongoing signal arrival");
                signalArrivalFigure->setVisible(false);
                auto iconFigure = signalArrivalFigure->getIconFigure();
                iconFigure->setImageName(imageName.substr(0, imageName.find_first_of(".")).c_str());
                iconFigure->setAnchor(cFigure::ANCHOR_NW);
                auto labelFigure = signalArrivalFigure->getLabelFigure();
                labelFigure->setPosition(iconFigure->getBounds().getSize() / 2);
                networkNodeVisualization->addAnnotation(signalArrivalFigure, signalArrivalFigure->getBounds().getSize(), signalArrivalPlacementHint, signalArrivalPlacementPriority);
                setSignalArrivalFigure(radio, signalArrivalFigure);
            }
        }
    }
}

void MediumCanvasVisualizer::handleRadioRemoved(const IRadio *radio)
{
    Enter_Method_Silent();
    auto transmissionFigure = removeSignalDepartureFigure(radio);
    if (transmissionFigure != nullptr) {
        auto module = const_cast<cModule *>(check_and_cast<const cModule *>(radio));
        auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(getContainingNode(module));
        networkNodeVisualization->removeAnnotation(transmissionFigure);
    }
    auto figure = removeSignalArrivalFigure(radio);
    if (figure != nullptr) {
        auto module = const_cast<cModule *>(check_and_cast<const cModule *>(radio));
        auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(getContainingNode(module));
        networkNodeVisualization->removeAnnotation(figure);
    }
}

void MediumCanvasVisualizer::handleSignalAdded(const ITransmission *transmission)
{
    Enter_Method_Silent();
    MediumVisualizerBase::handleSignalAdded(transmission);
    if (matchesTransmission(transmission)) {
        transmissions.push_back(transmission);
        if (displaySignals) {
            cGroupFigure *signalFigure = createSignalFigure(transmission);
            signalLayer->addFigure(signalFigure);
            setSignalFigure(transmission, signalFigure);
        }
        if (displaySignals || displayMainPowerDensityMap || displayPowerDensityMaps)
            setAnimationSpeed();
    }
}

void MediumCanvasVisualizer::handleSignalRemoved(const ITransmission *transmission)
{
    Enter_Method_Silent();
    MediumVisualizerBase::handleSignalRemoved(transmission);
    if (matchesTransmission(transmission)) {
        transmissions.erase(std::remove(transmissions.begin(), transmissions.end(), transmission));
        if (displaySignals) {
            cFigure *signalFigure = getSignalFigure(transmission);
            removeSignalFigure(transmission);
            if (signalFigure != nullptr)
                delete signalLayer->removeFigure(signalFigure);
        }
        if (displaySignals || displayMainPowerDensityMap || displayPowerDensityMaps)
            setAnimationSpeed();
    }
}

void MediumCanvasVisualizer::handleSignalDepartureStarted(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (matchesTransmission(transmission)) {
        if (displaySignals || displayMainPowerDensityMap || displayPowerDensityMaps)
            setAnimationSpeed();
        if (displaySignalDepartures) {
            auto transmitter = transmission->getTransmitter();
            if (!transmitter) return;
            auto figure = getSignalDepartureFigure(transmitter);
            auto networkNode = getContainingNode(check_and_cast<const cModule *>(transmitter));
            auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(networkNode);
            networkNodeVisualization->setAnnotationVisible(figure, true);
            auto labelFigure = check_and_cast<LabeledIconFigure *>(figure)->getLabelFigure();
            if (auto scalarTransmission = dynamic_cast<const ScalarTransmission *>(transmission)) {
                char tmp[32];
                sprintf(tmp, "%.4g dBW", fraction2dB(W(scalarTransmission->getPower()).get()));
                labelFigure->setText(tmp);
            }
            else
                labelFigure->setText("");
        }
    }
}

void MediumCanvasVisualizer::handleSignalDepartureEnded(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (matchesTransmission(transmission)) {
        if (displaySignals || displayMainPowerDensityMap || displayPowerDensityMaps)
            setAnimationSpeed();
        if (displaySignalDepartures) {
            auto transmitter = transmission->getTransmitter();
            if (!transmitter) return;
            auto figure = getSignalDepartureFigure(transmitter);
            auto networkNode = getContainingNode(check_and_cast<const cModule *>(transmitter));
            auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(networkNode);
            networkNodeVisualization->setAnnotationVisible(figure, false);
        }
    }
}

void MediumCanvasVisualizer::handleSignalArrivalStarted(const IReception *reception)
{
    Enter_Method_Silent();
    MediumVisualizerBase::handleSignalArrivalStarted(reception);
    if (matchesTransmission(reception->getTransmission())) {
        if (displaySignals || displayMainPowerDensityMap || displayPowerDensityMaps)
            setAnimationSpeed();
        if (displaySignalArrivals) {
            auto receiver = reception->getReceiver();
            if (networkNodeFilter.matches(check_and_cast<const cModule *>(receiver))) {
                auto figure = getSignalArrivalFigure(receiver);
                auto networkNode = getContainingNode(check_and_cast<const cModule *>(receiver));
                auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(networkNode);
                networkNodeVisualization->setAnnotationVisible(figure, true);
                auto labelFigure = check_and_cast<LabeledIconFigure *>(figure)->getLabelFigure();
                if (auto scalarReception = dynamic_cast<const ScalarReception *>(reception)) {
                    char tmp[32];
                    sprintf(tmp, "%.4g dBW", fraction2dB(W(scalarReception->getPower()).get()));
                    labelFigure->setText(tmp);
                }
                else
                    labelFigure->setText("");
            }
        }
        if (displayCommunicationHeat) {
            const ITransmission *transmission = reception->getTransmission();
            const IMediumLimitCache *mediumLimitCache = radioMedium->getMediumLimitCache();
            Coord min = mediumLimitCache->getMinConstraintArea();
            Coord max = mediumLimitCache->getMaxConstraintArea();
            Coord delta = max - min;
            int x1 = std::round((communicationHeatMapSize - 1) * ((transmission->getStartPosition().x - min.x) / delta.x));
            int y1 = std::round((communicationHeatMapSize - 1) * ((transmission->getStartPosition().y - min.x) / delta.y));
            int x2 = std::round((communicationHeatMapSize - 1) * ((reception->getStartPosition().x - min.x) / delta.x));
            int y2 = std::round((communicationHeatMapSize - 1) * ((reception->getStartPosition().y - min.y) / delta.y));
            communicationHeat->heatLine(x1, y1, x2, y2);
        }
    }
}

void MediumCanvasVisualizer::handleSignalArrivalEnded(const IReception *reception)
{
    Enter_Method_Silent();
    if (matchesTransmission(reception->getTransmission())) {
        if (displaySignals || displayMainPowerDensityMap || displayPowerDensityMaps)
            setAnimationSpeed();
        if (displaySignalArrivals) {
            auto receiver = reception->getReceiver();
            if (networkNodeFilter.matches(check_and_cast<const cModule *>(receiver))) {
                auto figure = getSignalArrivalFigure(receiver);
                auto networkNode = getContainingNode(check_and_cast<const cModule *>(receiver));
                auto networkNodeVisualization = networkNodeVisualizer->getNetworkNodeVisualization(networkNode);
                networkNodeVisualization->setAnnotationVisible(figure, false);
            }
        }
    }
}

#endif // WITH_RADIO

} // namespace visualizer
} // namespace inet

