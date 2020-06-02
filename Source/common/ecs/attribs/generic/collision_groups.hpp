#ifndef COLLISION_GROUPS_HPP
#define COLLISION_GROUPS_HPP


enum COLLISION_LAYER_BIT {
    COLLISION_DEFAULT_BIT = 0x01,
    COLLISION_ZONE_BIT    = 0x02,
    COLLISION_PROBE_BIT   = 0x04,
    COLLISION_BEACON_BIT  = 0x08,
    COLLISION_HITBOX_BIT  = 0x10,
    COLLISION_HURTBOX_BIT = 0x20
};

static const const char* s_collision_group_names[] = {
    "Default",
    "Zone",
    "Probe",
    "Beacon",
    "Hitbox",
    "Hurtbox"
};


#endif
