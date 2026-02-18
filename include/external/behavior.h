#ifndef PROTOACTOR_BEHAVIOR_H
#define PROTOACTOR_BEHAVIOR_H

#include "context.h"
#include <vector>
#include <functional>
#include <memory>

namespace protoactor {

/**
 * @brief Behavior represents a state machine for actors.
 * 
 * Behavior allows actors to change their message handling behavior dynamically.
 * It supports stacked behaviors (BecomeStacked) and unstacking (UnbecomeStacked).
 */
class Behavior {
public:
    using ReceiveFunc = std::function<void(std::shared_ptr<Context>)>;
    
    /**
     * @brief Create a new behavior.
     */
    static std::shared_ptr<Behavior> New();
    
    /**
     * @brief Replace the current behavior with a new one.
     * @param receive The new receive function
     */
    void Become(ReceiveFunc receive);
    
    /**
     * @brief Stack a new behavior on top of the current one.
     * @param receive The new receive function to stack
     */
    void BecomeStacked(ReceiveFunc receive);
    
    /**
     * @brief Unstack the topmost behavior, reverting to the previous one.
     */
    void UnbecomeStacked();
    
    /**
     * @brief Receive a message using the current behavior.
     * @param context The actor context
     */
    void Receive(std::shared_ptr<Context> context);
    
    /**
     * @brief Get the number of stacked behaviors.
     * @return Count of behaviors
     */
    size_t Length() const;

private:
    std::vector<ReceiveFunc> behaviors_;
    
    void Clear();
    ReceiveFunc Peek() const;
    void Push(ReceiveFunc receive);
    void Pop();
};

} // namespace protoactor

#endif // PROTOACTOR_BEHAVIOR_H
