# Meta-component that includes all capabilities
set(capability_dirs 
    cap_cron
    cap_hooks
    cap_heartbeat
    cap_pairing
    cap_secrets
    cap_ratelimit
    cap_react
    cap_context
    cap_vector
    cap_eventbus
    cap_watchdog
    cap_prompt
)

foreach(cap ${capability_dirs})
    add_subdirectory(${cap})
endforeach()
