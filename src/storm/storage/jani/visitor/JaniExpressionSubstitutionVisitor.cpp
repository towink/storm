#include "JaniExpressionSubstitutionVisitor.h"

#include "storm/exceptions/InvalidArgumentException.h"

namespace storm {
    
    namespace jani {
        storm::expressions::Expression substituteJaniExpression(storm::expressions::Expression const& expression, std::map<storm::expressions::Variable, storm::expressions::Expression> const& identifierToExpressionMap) {
            return storm::expressions::JaniExpressionSubstitutionVisitor<std::map<storm::expressions::Variable, storm::expressions::Expression>>(identifierToExpressionMap).substitute(expression);
        }

        storm::expressions::Expression substituteJaniExpression(storm::expressions::Expression const& expression, std::unordered_map<storm::expressions::Variable, storm::expressions::Expression> const& identifierToExpressionMap) {
            return storm::expressions::JaniExpressionSubstitutionVisitor<std::unordered_map<storm::expressions::Variable, storm::expressions::Expression>>(identifierToExpressionMap).substitute(expression);
        }
    }

    namespace expressions {
        
        template<typename MapType>
        JaniExpressionSubstitutionVisitor<MapType>::JaniExpressionSubstitutionVisitor(MapType const& variableToExpressionMapping) : SubstitutionVisitor<MapType>(variableToExpressionMapping) {
            // Intentionally left empty.
        }
        
        template<typename MapType>
        boost::any JaniExpressionSubstitutionVisitor<MapType>::visit(ValueArrayExpression const& expression, boost::any const& data) {
            uint64_t size = expression.size()->evaluateAsInt();
            std::vector<std::shared_ptr<BaseExpression const>> newElements;
            newElements.reserve(size);
            bool changed = false;
            for (uint64_t i = 0; i < size; ++i) {
                newElements.push_back(boost::any_cast<std::shared_ptr<BaseExpression const>>(expression.at(i)->accept(*this, data)));
                changed = changed || expression.at(i).get() != newElements.back().get();
            }
            
            if (changed) {
                return std::const_pointer_cast<BaseExpression const>(std::shared_ptr<BaseExpression>(new ValueArrayExpression(expression.getManager(), expression.getType(), newElements)));
            } else {
                return expression.getSharedPointer();
            }
        }

        template<typename MapType>
        boost::any JaniExpressionSubstitutionVisitor<MapType>::visit(ConstructorArrayExpression const& expression, boost::any const& data) {
          std::shared_ptr<BaseExpression const> newSize = boost::any_cast<std::shared_ptr<BaseExpression const>>(expression.size()->accept(*this, data));
          std::shared_ptr<BaseExpression const> elementExpression = boost::any_cast<std::shared_ptr<BaseExpression const>>(expression.getElementExpression()->accept(*this, data));
          STORM_LOG_THROW(this->variableToExpressionMapping.find(expression.getIndexVar()) == this->variableToExpressionMapping.end(), storm::exceptions::InvalidArgumentException, "substitution of the index variable of a constructorArrayExpression is not possible.");
            // If the arguments did not change, we simply push the expression itself.
            if (newSize.get() == expression.size().get() && elementExpression.get() == expression.getElementExpression().get()) {
                return expression.getSharedPointer();
            } else {
				return std::const_pointer_cast<BaseExpression const>(std::shared_ptr<BaseExpression>(new ConstructorArrayExpression(expression.getManager(), expression.getType(), newSize, expression.getIndexVar(), elementExpression)));
            }
        }

        template<typename MapType>
        boost::any JaniExpressionSubstitutionVisitor<MapType>::visit(ArrayAccessExpression const& expression, boost::any const& data) {
            std::shared_ptr<BaseExpression const> firstExpression = boost::any_cast<std::shared_ptr<BaseExpression const>>(expression.getFirstOperand()->accept(*this, data));
            std::shared_ptr<BaseExpression const> secondExpression = boost::any_cast<std::shared_ptr<BaseExpression const>>(expression.getSecondOperand()->accept(*this, data));

            // If the arguments did not change, we simply push the expression itself.
            if (firstExpression.get() == expression.getFirstOperand().get() && secondExpression.get() == expression.getSecondOperand().get()) {
                return expression.getSharedPointer();
            } else {
				return std::const_pointer_cast<BaseExpression const>(std::shared_ptr<BaseExpression>(new ArrayAccessExpression(expression.getManager(), expression.getType(), firstExpression, secondExpression)));
            }
        }

        template<typename MapType>
        boost::any JaniExpressionSubstitutionVisitor<MapType>::visit(FunctionCallExpression const& expression, boost::any const& data) {
            std::vector<std::shared_ptr<BaseExpression const>> newArguments;
            newArguments.reserve(expression.getNumberOfArguments());
            for (uint64_t i = 0; i < expression.getNumberOfArguments(); ++i) {
                newArguments.push_back(boost::any_cast<std::shared_ptr<BaseExpression const>>(expression.getArgument(i)->accept(*this, data)));
            }
            return std::const_pointer_cast<BaseExpression const>(std::shared_ptr<BaseExpression>(new FunctionCallExpression(expression.getManager(), expression.getType(), expression.getFunctionIdentifier(), newArguments)));
        }

        // Explicitly instantiate the class with map and unordered_map.
		template class JaniExpressionSubstitutionVisitor<std::map<Variable, Expression>>;
		template class JaniExpressionSubstitutionVisitor<std::unordered_map<Variable, Expression>>;
    
    }
}
