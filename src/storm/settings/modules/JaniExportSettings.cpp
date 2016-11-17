#include "JaniExportSettings.h"

#include "src/storm/settings/SettingsManager.h"
#include "src/storm/settings/SettingMemento.h"
#include "src/storm/settings/Option.h"
#include "src/storm/settings/OptionBuilder.h"
#include "src/storm/settings/ArgumentBuilder.h"
#include "src/storm/settings/Argument.h"

#include "src/storm/exceptions/InvalidSettingsException.h"

namespace storm {
    namespace settings {
        namespace modules {
            const std::string JaniExportSettings::moduleName = "exportJani";
            
            const std::string JaniExportSettings::janiFileOptionName = "jani-output";
            const std::string JaniExportSettings::janiFileOptionShortName = "output";
            const std::string JaniExportSettings::standardCompliantOptionName = "standard-compliant";
            const std::string JaniExportSettings::standardCompliantOptionShortName = "standard";

            
            JaniExportSettings::JaniExportSettings() : ModuleSettings(moduleName) {
                this->addOption(storm::settings::OptionBuilder(moduleName, janiFileOptionName, false, "Destination for the jani model.").setShortName(janiFileOptionShortName).addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "path to file").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, standardCompliantOptionName, false, "Export in standard compliant variant.").setShortName(standardCompliantOptionShortName).build());
            }
            
            bool JaniExportSettings::isJaniFileSet() const {
                return this->getOption(janiFileOptionName).getHasOptionBeenSet();
            }
            
            std::string JaniExportSettings::getJaniFilename() const {
                return this->getOption(janiFileOptionName).getArgumentByName("filename").getValueAsString();
            }
            
            bool JaniExportSettings::isExportAsStandardJaniSet() const {
                return this->getOption(standardCompliantOptionName).getHasOptionBeenSet();
            }
            
            void JaniExportSettings::finalize() {
                
            }
            
            bool JaniExportSettings::check() const {
                return true;
            }
        }
    }
}