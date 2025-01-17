#include "storm-gspn/builder/ExplicitGspnModelBuilder.h"
#include "storm-gspn/parser/GspnParser.h"
#include "storm-gspn/storage/gspn/GSPN.h"
#include "storm-gspn/storage/gspn/GspnBuilder.h"
#include "storm-gspn/builder/JaniGSPNBuilder.h"
#include "storm-gspn/api/storm-gspn.h"

#include "storm/utility/macros.h"
#include "storm/utility/initialize.h"

#include "api/storm.h"

#include "storm-parsers/api/storm-parsers.h"
#include "storm-cli-utilities/cli.h"

#include "storm-parsers/parser/FormulaParser.h"

#include "storm/storage/expressions/ExpressionManager.h"
#include "storm/storage/jani/Model.h"
#include "storm/storage/jani/visitor/JSONExporter.h"
#include <fstream>
#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

#include "storm/exceptions/FileIoException.h"

#include "storm/settings/modules/GeneralSettings.h"
#include "storm/settings/modules/IOSettings.h"
#include "storm-gspn/settings/modules/GSPNSettings.h"
#include "storm-gspn/settings/modules/GSPNExportSettings.h"
#include "storm/settings/modules/CoreSettings.h"
#include "storm/settings/modules/DebugSettings.h"
#include "storm-conv/settings/modules/JaniExportSettings.h"
#include "storm/settings/modules/ResourceSettings.h"


/*!
 * Initialize the settings manager.
 */
void initializeSettings() {
    storm::settings::mutableManager().setName("Storm-GSPN", "storm-gspn");
    
    // Register all known settings modules.
    storm::settings::addModule<storm::settings::modules::GeneralSettings>();
    storm::settings::addModule<storm::settings::modules::IOSettings>();
    storm::settings::addModule<storm::settings::modules::GSPNSettings>();
    storm::settings::addModule<storm::settings::modules::GSPNExportSettings>();
    storm::settings::addModule<storm::settings::modules::CoreSettings>();
    storm::settings::addModule<storm::settings::modules::DebugSettings>();
    storm::settings::addModule<storm::settings::modules::JaniExportSettings>();
    storm::settings::addModule<storm::settings::modules::ResourceSettings>();
}


int main(const int argc, const char **argv) {
    try {
        storm::utility::setUp();
        storm::cli::printHeader("Storm-GSPN", argc, argv);
        initializeSettings();
        
        bool optionsCorrect = storm::cli::parseOptions(argc, argv);
        if (!optionsCorrect) {
            return -1;
        }

        // Start by setting some urgent options (log levels, resources, etc.)
        storm::cli::setUrgentOptions();

        auto gspnSettings = storm::settings::getModule<storm::settings::modules::GSPNSettings>();

        // parse gspn from file
        if (!gspnSettings.isGspnFileSet()) {
            // If no gspn file is given, nothing needs to be done.
            return 0;
        }
        
        std::string constantDefinitionString = "";
        if (gspnSettings.isConstantsSet()) {
            constantDefinitionString = gspnSettings.getConstantDefinitionString();
        }
        
        auto parser = storm::parser::GspnParser();
        auto gspn = parser.parse(gspnSettings.getGspnFilename(), constantDefinitionString);

        std::string formulaString = "";
        if (storm::settings::getModule<storm::settings::modules::IOSettings>().isPropertySet()) {
            formulaString = storm::settings::getModule<storm::settings::modules::IOSettings>().getProperty();
        }
        boost::optional<std::set<std::string>> propertyFilter;
        storm::parser::FormulaParser formulaParser(gspn->getExpressionManager());
        std::vector<storm::jani::Property> properties  = storm::api::parseProperties(formulaParser, formulaString, propertyFilter);
        properties = storm::api::substituteConstantsInProperties(properties, gspn->getConstantsSubstitution());
        
        if (!gspn->isValid()) {
            STORM_LOG_ERROR("The gspn is not valid.");
        }
        
        if(gspnSettings.isCapacitiesFileSet()) {
            auto capacities = storm::api::parseCapacitiesList(gspnSettings.getCapacitiesFilename(), *gspn);
            gspn->setCapacities(capacities);
        } else if (gspnSettings.isCapacitySet()) {
            uint64_t capacity = gspnSettings.getCapacity();
            std::unordered_map<std::string, uint64_t> capacities;
            for (auto const& place : gspn->getPlaces()) {
                capacities.emplace(place.getName(), capacity);
            }
            gspn->setCapacities(capacities);
        }

        storm::api::handleGSPNExportSettings(*gspn, [&](storm::builder::JaniGSPNBuilder const&) { return properties; });
        
        delete gspn;
        return 0;
        
//        // construct ma
//        auto builder = storm::builder::ExplicitGspnModelBuilder<>();
//        auto ma = builder.translateGspn(gspn, formula);
//
        // All operations have now been performed, so we clean up everything and terminate.
        storm::utility::cleanUp();
        return 0;
    } catch (storm::exceptions::BaseException const& exception) {
        STORM_LOG_ERROR("An exception caused Storm to terminate. The message of the exception is: " << exception.what());
        return 1;
    } catch (std::exception const& exception) {
        STORM_LOG_ERROR("An unexpected exception occurred and caused Storm to terminate. The message of this exception is: " << exception.what());
        return 2;
    }
}
