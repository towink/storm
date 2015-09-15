#include "src/storage/prism/menu_games/AbstractCommand.h"

#include <boost/iterator/transform_iterator.hpp>

#include "src/storage/prism/menu_games/AbstractionExpressionInformation.h"
#include "src/storage/prism/menu_games/AbstractionDdInformation.h"

#include "src/storage/dd/CuddDdManager.h"
#include "src/storage/dd/CuddAdd.h"

#include "src/storage/prism/Command.h"
#include "src/storage/prism/Update.h"

namespace storm {
    namespace prism {
        namespace menu_games {
            template <storm::dd::DdType DdType, typename ValueType>
            AbstractCommand<DdType, ValueType>::AbstractCommand(storm::prism::Command const& command, AbstractionExpressionInformation const& expressionInformation, AbstractionDdInformation<DdType, ValueType> const& ddInformation, storm::utility::solver::SmtSolverFactory const& smtSolverFactory) : smtSolver(smtSolverFactory.create(expressionInformation.expressionManager)), expressionInformation(expressionInformation), ddInformation(ddInformation), command(command), variablePartition(expressionInformation.variables), relevantPredicatesAndVariables(), cachedDd(std::make_pair(ddInformation.manager->getBddZero(), 0)), decisionVariables() {

                // Make the second component of relevant predicates have the right size.
                relevantPredicatesAndVariables.second.resize(command.getNumberOfUpdates());
                
                // Assert all range expressions to enforce legal variable values.
                for (auto const& rangeExpression : expressionInformation.rangeExpressions) {
                    smtSolver->add(rangeExpression);
                }
            }
            
            template <storm::dd::DdType DdType, typename ValueType>
            std::pair<std::set<uint_fast64_t>, std::set<uint_fast64_t>> AbstractCommand<DdType, ValueType>::computeRelevantPredicates(std::vector<storm::prism::Assignment> const& assignments) const {
                std::pair<std::set<uint_fast64_t>, std::set<uint_fast64_t>> result;
                
                // To start with, all predicates related to the guard are relevant source predicates.
                result.first = variablePartition.getExpressionsUsingVariables(command.get().getGuardExpression().getVariables());
                
                std::set<storm::expressions::Variable> assignedVariables;
                for (auto const& assignment : assignments) {
                    // Also, variables appearing on the right-hand side of an assignment are relevant for source state.
                    auto const& rightHandSidePredicates = variablePartition.getExpressionsUsingVariables(assignment.getExpression().getVariables());
                    result.first.insert(rightHandSidePredicates.begin(), rightHandSidePredicates.end());
                    
                    // Variables that are being assigned are relevant for the successor state.
                    storm::expressions::Variable const& assignedVariable = assignment.getVariable();
                    auto const& leftHandSidePredicates = variablePartition.getExpressionsUsingVariable(assignedVariable);
                    result.second.insert(leftHandSidePredicates.begin(), leftHandSidePredicates.end());
                    
                    // Keep track of all assigned variables, so we can find the related predicates later.
                    assignedVariables.insert(assignedVariable);
                }
                
                auto const& predicatesRelatedToAssignedVariable = variablePartition.getRelatedExpressions(assignedVariables);
                result.first.insert(predicatesRelatedToAssignedVariable.begin(), predicatesRelatedToAssignedVariable.end());
                
                return result;
            }
            
            template <storm::dd::DdType DdType, typename ValueType>
            std::pair<std::set<uint_fast64_t>, std::vector<std::set<uint_fast64_t>>> AbstractCommand<DdType, ValueType>::computeRelevantPredicates() const {
                std::pair<std::set<uint_fast64_t>, std::vector<std::set<uint_fast64_t>>> result;

                for (auto const& update : command.get().getUpdates()) {
                    std::pair<std::set<uint_fast64_t>, std::set<uint_fast64_t>> relevantUpdatePredicates = computeRelevantPredicates(update.getAssignments());
                    result.first.insert(relevantUpdatePredicates.first.begin(), relevantUpdatePredicates.first.end());
                    result.second.push_back(relevantUpdatePredicates.second);
                }
                
                return result;
            }
            
            template <storm::dd::DdType DdType, typename ValueType>
            bool AbstractCommand<DdType, ValueType>::relevantPredicatesChanged(std::pair<std::set<uint_fast64_t>, std::vector<std::set<uint_fast64_t>>> const& newRelevantPredicates) const {
                if (newRelevantPredicates.first.size() > relevantPredicatesAndVariables.first.size()) {
                    return true;
                }
                
                for (uint_fast64_t index = 0; index < command.get().getNumberOfUpdates(); ++index) {
                    if (newRelevantPredicates.second[index].size() > relevantPredicatesAndVariables.second[index].size()) {
                        return true;
                    }
                }
                
                return false;
            }
            
            template <storm::dd::DdType DdType, typename ValueType>
            std::vector<std::pair<storm::expressions::Variable, uint_fast64_t>> AbstractCommand<DdType, ValueType>::declareNewVariables(std::vector<std::pair<storm::expressions::Variable, uint_fast64_t>> const& oldRelevantPredicates, std::set<uint_fast64_t> const& newRelevantPredicates) {
                std::vector<std::pair<storm::expressions::Variable, uint_fast64_t>> result;
                
                auto oldIt = oldRelevantPredicates.begin();
                auto oldIte = oldRelevantPredicates.end();
                for (auto newIt = newRelevantPredicates.begin(), newIte = newRelevantPredicates.end(); newIt != newIte; ++newIt) {
                    // If the new variable does not yet exist as a source variable, we create it now.
                    if (oldIt == oldIte || oldIt->second != *newIt) {
                        result.push_back(std::make_pair(expressionInformation.expressionManager.declareFreshBooleanVariable(), *newIt));
                    }
                }
                
                return result;
            }
            
            template <storm::dd::DdType DdType, typename ValueType>
            void AbstractCommand<DdType, ValueType>::addMissingPredicates(std::pair<std::set<uint_fast64_t>, std::vector<std::set<uint_fast64_t>>> const& newRelevantPredicates) {
                // Determine and add new relevant source predicates.
                std::vector<std::pair<storm::expressions::Variable, uint_fast64_t>> newSourceVariables = declareNewVariables(relevantPredicatesAndVariables.first, newRelevantPredicates.first);
                for (auto const& element : newSourceVariables) {
                    smtSolver->add(storm::expressions::iff(element.first, expressionInformation.predicates[element.second]));
                    decisionVariables.push_back(element.first);
                }
                
                // Insert the new variables into the record of relevant source variables.
                relevantPredicatesAndVariables.first.insert(relevantPredicatesAndVariables.first.end(), newSourceVariables.begin(), newSourceVariables.end());
                std::sort(relevantPredicatesAndVariables.first.begin(), relevantPredicatesAndVariables.first.end(), [] (std::pair<storm::expressions::Variable, uint_fast64_t> const& first, std::pair<storm::expressions::Variable, uint_fast64_t> const& second) { return first.second < second.second; } );
                
                // Do the same for every update.
                for (uint_fast64_t index = 0; index < command.get().getNumberOfUpdates(); ++index) {
                    std::vector<std::pair<storm::expressions::Variable, uint_fast64_t>> newSuccessorVariables = declareNewVariables(relevantPredicatesAndVariables.second[index], newRelevantPredicates.second[index]);
                    for (auto const& element : newSuccessorVariables) {
                        smtSolver->add(storm::expressions::iff(element.first, expressionInformation.predicates[element.second].substitute(command.get().getUpdate(index).getAsVariableToExpressionMap())));
                        decisionVariables.push_back(element.first);
                    }
                    
                    relevantPredicatesAndVariables.second[index].insert(relevantPredicatesAndVariables.second[index].end(), newSuccessorVariables.begin(), newSuccessorVariables.end());
                    std::sort(relevantPredicatesAndVariables.second[index].begin(), relevantPredicatesAndVariables.second[index].end(), [] (std::pair<storm::expressions::Variable, uint_fast64_t> const& first, std::pair<storm::expressions::Variable, uint_fast64_t> const& second) { return first.second < second.second; } );
                }
            }
            
            template <storm::dd::DdType DdType, typename ValueType>
            storm::dd::Bdd<DdType> AbstractCommand<DdType, ValueType>::getSourceStateBdd(storm::solver::SmtSolver::ModelReference const& model) const {
                storm::dd::Bdd<DdType> result = ddInformation.manager->getBddOne();
                for (auto const& variableIndexPair : relevantPredicatesAndVariables.first) {
                    if (model.getBooleanValue(variableIndexPair.first)) {
                        result &= ddInformation.predicateBdds[variableIndexPair.second].first;
                    } else {
                        result &= !ddInformation.predicateBdds[variableIndexPair.second].first;
                    }
                }
                return result;
            }
            
            template <storm::dd::DdType DdType, typename ValueType>
            storm::dd::Bdd<DdType> AbstractCommand<DdType, ValueType>::getDistributionBdd(storm::solver::SmtSolver::ModelReference const& model) const {
                storm::dd::Bdd<DdType> result = ddInformation.manager->getBddZero();
                
                for (uint_fast64_t updateIndex = 0; updateIndex < command.get().getNumberOfUpdates(); ++updateIndex) {
                    storm::dd::Bdd<DdType> updateBdd = ddInformation.manager->getBddOne();

                    for (auto const& variableIndexPair : relevantPredicatesAndVariables.second[updateIndex]) {
                        if (model.getBooleanValue(variableIndexPair.first)) {
                            updateBdd &= ddInformation.predicateBdds[variableIndexPair.second].second;
                        } else {
                            updateBdd &= !ddInformation.predicateBdds[variableIndexPair.second].second;
                        }
                        updateBdd &= ddInformation.manager->getEncoding(ddInformation.updateDdVariable, updateIndex);
                    }
                    
                    // Compute the identities that are missing for this update.
                    auto firstIt = relevantPredicatesAndVariables.first.begin();
                    auto firstIte = relevantPredicatesAndVariables.first.end();
                    auto secondIt = relevantPredicatesAndVariables.second[updateIndex].begin();
                    auto secondIte = relevantPredicatesAndVariables.second[updateIndex].end();
                    
                    for (; firstIt != firstIte; ++firstIt) {
                        if (secondIt == secondIte || firstIt->second != secondIt->second) {
                            result &= ddInformation.predicateIdentities[firstIt->second];
                        } else if (secondIt != secondIte) {
                            ++secondIt;
                        }
                    }
                    
                    result |= updateBdd;
                }
                
                return result;
            }
                        
            template <storm::dd::DdType DdType, typename ValueType>
            std::pair<storm::dd::Bdd<DdType>, uint_fast64_t> AbstractCommand<DdType, ValueType>::computeDd() {
                // First, we check whether there is work to be done by recomputing the relevant predicates and checking
                // whether they changed.
                std::pair<std::set<uint_fast64_t>, std::vector<std::set<uint_fast64_t>>> newRelevantPredicates = this->computeRelevantPredicates();

                // If the DD does not need recomputation, we can return the cached result.
                bool recomputeDd = this->relevantPredicatesChanged(newRelevantPredicates);
                if (!recomputeDd) {
                    // FIXME: multiply identity of new predicates
                    return cachedDd;
                }
                
                // If the DD needs recomputation, it is because of new relevant predicates, so we need to assert the appropriate clauses in the solver.
                addMissingPredicates(newRelevantPredicates);

                // Create a mapping from source state DDs to their distributions.
                std::unordered_map<storm::dd::Bdd<DdType>, std::vector<storm::dd::Bdd<DdType>>> sourceToDistributionsMap;
                smtSolver->allSat(decisionVariables, [&sourceToDistributionsMap,this] (storm::solver::SmtSolver::ModelReference const& model) { sourceToDistributionsMap[getSourceStateBdd(model)].push_back(getDistributionBdd(model)); return true; } );
                
                // Now we search for the maximal number of choices of player 2 to determine how many DD variables we
                // need to encode the nondeterminism.
                uint_fast64_t maximalNumberOfChoices = 0;
                for (auto const& sourceDistributionsPair : sourceToDistributionsMap) {
                    maximalNumberOfChoices = std::max(maximalNumberOfChoices, static_cast<uint_fast64_t>(sourceDistributionsPair.second.size()));
                }
                
                uint_fast64_t numberOfVariablesNeeded = static_cast<uint_fast64_t>(std::ceil(std::log2(maximalNumberOfChoices)));
                
                // Finally, build overall result.
                storm::dd::Bdd<DdType> resultBdd = ddInformation.manager->getBddZero();
                for (auto const& sourceDistributionsPair : sourceToDistributionsMap) {
                    uint_fast64_t distributionIndex = 0;
                    storm::dd::Bdd<DdType> allDistributions = ddInformation.manager->getBddZero();
                    for (auto const& distribution : sourceDistributionsPair.second) {
                        allDistributions |= distribution && ddInformation.encodeDistributionIndex(numberOfVariablesNeeded, distributionIndex);
                    }
                    resultBdd |= sourceDistributionsPair.first && allDistributions;
                }
                
                // Cache the result before returning it.
                cachedDd = std::make_pair(resultBdd, numberOfVariablesNeeded);
                return cachedDd;
            }
            
            template class AbstractCommand<storm::dd::DdType::CUDD, double>;
        }
    }
}