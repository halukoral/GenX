#pragma once
#include "Component.h"
#include "ECS.h"

class System
{
public:
	virtual void Update(class ECS& ecs, float dt) = 0;
	virtual ~System() = default;
};

class MovementSystem final : public System
{
public:
	void Update(ECS& ecs, float dt) override
	{
		ecs.Query<Position, Velocity>([dt](Entity e, Position& pos, Velocity& vel)
		{
			pos.x += vel.dx * dt;
			pos.y += vel.dy * dt;
		});
	}
};
