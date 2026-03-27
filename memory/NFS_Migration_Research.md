# StuntRally3 to NFS Most Wanted/Carbon - Feature Implementation Plan

## Current StuntRally3 Physics Architecture

### Collision System (Bullet Physics)
- **Collision World**: Uses Bullet Physics with discrete dynamics
- **Car Collision Group**: Cars are in collision group `COL_CAR = (1<<2)` with mask `255 - COL_CAR`
- **Wheel Triggers**: Wheels use `CF_NO_CONTACT_RESPONSE` flag to avoid collision response
- **Collision Callback**: Custom `IntTickCallback` processes all contact points
  - Collectibles detection
  - Fluid field interactions
  - Object collisions with sound playback
  - Car-to-car and car-to-terrain damage calculation

### Car Physics Components
1. **CARDYNAMICS** - Main physics simulation
   - Driveline: engine, clutch, transmission, differential
   - Suspension: spring/damper systems
   - Wheels: tire friction models
   - Buoyancy: fluid immersion physics
   - Aerodynamics: downforce/drag calculations

2. **CAR** - Game integration layer
   - Input handling
   - Sound system
   - Collision detection (via CARDYNAMICS)
   - Damage modeling

3. **Collision Contact Processing**
   - Contact point impulse-based force calculation
   - Damage factors: front/side/top impact normals
   - Spark emission parameters
   - Sound force calculation

## Blackbox NFS Most Wanted/Carbon Features

### Key Differences from StuntRally3

| Feature | StuntRally3 | NFS MW/C |
|---------|-------------|----------|
| **Collision System** | Bullet Physics | Custom rigid body |
| **Car-to-Car Collision** | Contact response | Elastic collision |
| **Damage Modeling** | Contact impulse based | BumpZone system |
| **Physics Callbacks** | Per-contact | Per-frame integration |
| **Tire Friction** | Contact point friction | Slip-based |

### Car-to-Car Collision Implementation

#### 1. Collision Detection (NFS Style)

**Blackbox Approach:**
```cpp
// BumpZone system - pre-defined collision zones on car body
struct BumpZone {
    Vector3 position;
    Vector3 normal;
    float radius;
    float damageMultiplier;
    float soundThreshold;
};

class CAR {
    std::vector<BumpZone> bumpZones;
    float totalDamage;
    float deformation;
};
```

**Implementation Steps:**
1. Define collision zones on car model (front, sides, top)
2. Use sphere/capsule collision for each zone
3. Check all car pairs for overlaps
4. Apply impulse-based collision response
5. Calculate damage based on relative velocity

#### 2. Collision Response

**StuntRally3 (Bullet):**
```cpp
// Uses Bullet's contact solver
btScalar impulse = pt.getAppliedImpulse();
float damage = impulse * timeStep * damageFactor;
```

**NFS Style (Custom):**
```cpp
// Elastic collision with energy-based damage
void ResolveCarCollision(CAR* a, CAR* b) {
    Vector3 relativeVel = a->velocity - b->velocity;
    float impactSpeed = relativeVel.length();
    
    // Elastic collision formula
    Vector3 normal = (b->position - a->position).normalize();
    float restitution = 0.3f; // NFS-style elasticity
    
    float impulse = (1 + restitution) * impactSpeed / 2.0f;
    
    // Apply velocities
    a->velocity -= normal * impulse;
    b->velocity += normal * impulse;
    
    // Calculate damage (NFS-style)
    float damage = impactSpeed * impactSpeed * 0.01f;
    a->damage += damage * a->damageFactor;
    b->damage += damage * b->damageFactor;
}
```

### Feature Implementation Roadmap

#### Phase 1: Collision System Override
1. **Create custom collision manager**
   - Replace Bullet collision callbacks with NFS-style system
   - Implement bumpZone detection
   - Add elastic collision response

2. **Car-to-car collision**
   - Define collision zones per car model
   - Implement broad-phase collision detection
   - Add narrow-phase collision resolution

#### Phase 2: Damage System
1. **BumpZone system**
   - Front bumper (high damage)
   - Side doors (medium damage)
   - Roof/top (low damage)
   - Rear (medium damage)

2. **Damage propagation**
   - Car deformation based on impact direction
   - Part removal (bumpers, doors)
   - Engine damage affecting performance

#### Phase 3: Visual Feedback
1. **Particle effects**
   - Sparks on impact
   - Smoke from damaged areas
   - Particles from removed parts

2. **Car deformation**
   - Vertex animation for dents
   - Damage masks for textures
   - Part removal animations

#### Phase 4: Game Mechanics
1. **Racing modes**
   - Street races (no damage)
   - Drift competitions
   - Destruction races

2. **Career system**
   - Car repair costs
   - Insurance system
   - Damage affecting performance

### Implementation Details

#### Collision Detection Algorithm
```cpp
class CollisionManager {
    std::vector<Car*> cars;
    
    void CheckCarCollisions() {
        // Broad phase: spatial partitioning (grid/octree)
        for (auto& carPair : potentialCollisions) {
            Car* a = carPair.first;
            Car* b = carPair.second;
            
            // Narrow phase: check bumpZones
            for (auto& zoneA : a->bumpZones) {
                for (auto& zoneB : b->bumpZones) {
                    float dist = distance(zoneA.position, zoneB.position);
                    if (dist < zoneA.radius + zoneB.radius) {
                        ResolveCollision(a, b, zoneA, zoneB, dist);
                    }
                }
            }
        }
    }
    
    void ResolveCollision(CAR* a, CAR* b, BumpZone& zoneA, BumpZone& zoneB, float dist) {
        // Calculate collision normal
        Vector3 normal = (b->position - a->position).normalize();
        
        // Calculate impact speed
        float impactSpeed = dot(a->velocity - b->velocity, normal);
        
        if (impactSpeed < 0) {
            // Apply elastic collision response
            float impulse = (1 + 0.3f) * abs(impactSpeed) / (a->mass + b->mass);
            
            a->velocity -= normal * impulse * (1.0f / a->mass);
            b->velocity += normal * impulse * (1.0f / b->mass);
            
            // Calculate damage
            float damage = abs(impactSpeed) * 0.1f;
            ApplyDamage(a, zoneA, damage);
            ApplyDamage(b, zoneB, damage);
        }
    }
};
```

#### Damage System
```cpp
void ApplyDamage(CAR* car, BumpZone& zone, float damage) {
    // Clamp damage
    damage = clamp(damage, 0.0f, 100.0f);
    
    // Apply to car total damage
    car->totalDamage += damage;
    car->totalDamage = clamp(car->totalDamage, 0.0f, 100.0f);
    
    // Zone-specific damage
    zone.damage += damage;
    
    // Determine damage type
    if (damage > 50) {
        // Major damage: remove part
        RemovePart(zone.partType);
    } else if (damage > 20) {
        // Minor damage: deformation
        car->deformation += damage * 0.1f;
    }
}
```

### Key Research Questions

1. **Collision Detection Method**
   - Should we use Bullet's broadphase with custom narrowphase?
   - Or implement completely custom collision system?
   - What spatial partitioning to use for many cars?

2. **Damage Modeling**
   - NFS-style bumpZones vs StuntRally3 contact impulse?
   - How to handle multiple simultaneous collisions?
   - Car deformation vs part removal?

3. **Performance Considerations**
   - How many cars to support simultaneously?
   - Spatial partitioning requirements
   - Collision cache strategies

### Next Steps

1. **Prototype collision system**
   - Implement bumpZone detection
   - Test with 2 cars
   - Measure performance

2. **Damage system**
   - Create damage visualization
   - Implement part removal
   - Test damage progression

3. **Integration**
   - Replace Bullet collision callbacks
   - Test with existing race tracks
   - Tune collision parameters

### References

- StuntRally3 collision_world.cpp implementation
- Bullet Physics documentation
- NFS Most Wanted physics analysis
- Car collision research papers
