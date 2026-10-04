#include "mod/MyMod.h"
#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerUseItemEvent.h>
#include <ll/api/event/player/PlayerTickEvent.h>
#include 
#include <ll/api/item/ItemStack.h>
#include <pl/Input.hpp>
#include <atomic>

std::atomic_bool gTouchToggle{false};
std::atomic_bool gMouseRightDown{false};
std::atomic_bool gMouseCallbackRegistered{false};

void registerMouseOnce() {
    bool expected = false;
    if (!gMouseCallbackRegistered.compare_exchange_strong(expected, true)) return;
    pl::input::registerMouseCallback([](const pl::input::MouseEvent& event) -> bool {
        if (event.button == 2) {
            gMouseRightDown.store(event.isDown, std::memory_order_relaxed);
        }
        return false;
    });
}

bool requestedBlock(Player& player) noexcept {
    if (player.isRiding()) {
        return false; 
    }
    return (gTouchToggle.load(std::memory_order_relaxed) || gMouseRightDown.load(std::memory_order_relaxed));
}

bool strcontains(const std::string& text, const std::string& search) {
    return text.find(search) != std::string::npos;
}

namespace my_mod {

MyMod &MyMod::getInstance() {
    static MyMod instance;
    return instance;
}

MyMod::MyMod() : mSelf(*ll::mod::NativeMod::current()) {}

bool MyMod::load() {
    auto &self = getSelf();
    self.getLogger().info("Loading resilient Java Shield Mod...");
    registerMouseOnce();
    return true;
}

bool MyMod::enable() {
    auto &self = getSelf();
    self.getLogger().info("Enabling Event Listeners...");

    auto& eventBus = ll::event::EventBus::getInstance();

    eventBus.emplaceListener<ll::event::PlayerUseItemEvent>(
        [](ll::event::PlayerUseItemEvent& ev) {
            auto& player = ev.self();
            auto& stack = player.getSelectedItem();
            
            if (stack.isValid()) {
                std::string name = stack.getRawNameId();
                if (strcontains(name, "spear") || name != "minecraft:shield") {
                    gTouchToggle.store(false, std::memory_order_relaxed);
                }
            }
        }
    );

    eventBus.emplaceListener<ll::event::PlayerTickEvent>(
        [](ll::event::PlayerTickEvent& ev) {
            auto& player = ev.self();
            
            auto& mainhandItem = player.getSelectedItem();
            if (mainhandItem.isValid() && strcontains(mainhandItem.getRawNameId(), "spear")) {
                gTouchToggle.store(false, std::memory_order_relaxed);
            }

            bool shouldBlock = requestedBlock(player);

            if (player.isSneaking()) {
                if (!shouldBlock) {
                    player.setBlocking(false); 
                }
            } else if (shouldBlock) {
                player.setBlocking(true);
                player.setSprinting(false);
            }
        }
    );

    return true;
}

bool MyMod::disable() {
    getSelf().getLogger().info("Disabling Event Listeners...");
    
    gTouchToggle.store(false, std::memory_order_relaxed);
    gMouseRightDown.store(false, std::memory_order_relaxed);
    gMouseCallbackRegistered.store(false, std::memory_order_relaxed);

    return true;
}

bool MyMod::unload() {
    getSelf().getLogger().info("Unloading...");
    return true;
}

} // namespace my_mod
