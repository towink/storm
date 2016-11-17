#include "src/storm/storage/expressions/BooleanLiteralExpression.h"
#include "src/storm/storage/expressions/ExpressionManager.h"
#include "src/storm/storage/expressions/ExpressionVisitor.h"

namespace storm {
    namespace expressions {
        BooleanLiteralExpression::BooleanLiteralExpression(ExpressionManager const& manager, bool value) : BaseExpression(manager, manager.getBooleanType()), value(value) {
            // Intentionally left empty.
        }
        
        bool BooleanLiteralExpression::evaluateAsBool(Valuation const* valuation) const {
            return this->getValue();
        }
        
        bool BooleanLiteralExpression::isLiteral() const {
            return true;
        }
        
        bool BooleanLiteralExpression::isTrue() const {
            return this->getValue() == true;
        }
        
        bool BooleanLiteralExpression::isFalse() const {
            return this->getValue() == false;
        }
        
        void BooleanLiteralExpression::gatherVariables(std::set<storm::expressions::Variable>& variables) const {
            return;
		}
        
        std::shared_ptr<BaseExpression const> BooleanLiteralExpression::simplify() const {
            return this->shared_from_this();
        }
        
        boost::any BooleanLiteralExpression::accept(ExpressionVisitor& visitor, boost::any const& data) const {
            return visitor.visit(*this, data);
        }
        
        bool BooleanLiteralExpression::getValue() const {
            return this->value;
        }
        
        void BooleanLiteralExpression::printToStream(std::ostream& stream) const {
            stream << (this->getValue() ? "true" : "false");
        }
    }
}