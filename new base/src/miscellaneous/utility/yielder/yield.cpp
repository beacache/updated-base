#include "yield.h"
#include <mutex>
#include <globals.h>

void yielder::run()
{
	while (true)
	{
		std::function<void()> YieldingRequest;
		{
			std::lock_guard<std::mutex> lock();
			if (!module::rbx::globals.yielder_queue.empty())
			{
				YieldingRequest = module::rbx::globals.yielder_queue.front();
				module::rbx::globals.yielder_queue.pop();
			}
		}

		if (YieldingRequest)
		{
			YieldingRequest();
		}
		else
		{
			break;
		}
	}
}

int yielder::execution(lua_State* L, const std::function<yielded()>& YieldingClosure)
{
	lua_pushthread(L);
	int YieldedThreadRef = lua_ref(L, -1);

	std::thread([=]
		{
			yielded ResumeFunction = YieldingClosure();

			{
				std::lock_guard<std::mutex> lock(module::rbx::globals.yield_mutex);
				module::rbx::globals.yielder_queue.emplace([=]() -> void
					{

						lua_State* ML = module::rbx::globals.exploit_thread;
						int Top = lua_gettop(ML);

						lua_getglobal(ML, "task");
						if (lua_istable(ML, -1))
						{
							lua_getfield(ML, -1, "defer");

							lua_getref(ML, YieldedThreadRef);

							int nRes = ResumeFunction(L);
							if (nRes > 0)
							{
								lua_xmove(L, ML, nRes);
							}

							if (lua_pcall(ML, 1 + nRes, 0, 0) != LUA_OK)
							{

							}
						}
						else
						{

						}

						lua_settop(ML, Top);

						lua_unref(L, YieldedThreadRef);
					});
			}
		}).detach();

	return lua_yield(L, NULL);
}