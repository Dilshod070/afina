#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <cstring>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
	char StackEndsHere;
	if (&StackEndsHere > this->StackBottom) {
		ctx.Hight = &StackEndsHere;
		ctx.Low = this->StackBottom;
	} else {
		ctx.Low = &StackEndsHere;
		ctx.Hight = this->StackBottom;
	}
	uint32_t stack_size = ctx.Hight - ctx.Low;
	if ((stack_size > std::get<1>(ctx.Stack)) || (stack_size < std::get<1>(ctx.Stack) / 2)) {
		if (std::get<0>(ctx.Stack) != nullptr) {
			delete[] std::get<0>(ctx.Stack);
		}
		char *newStack = new char[stack_size];
		ctx.Stack = std::make_tuple(newStack, stack_size);
	}
	std::memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);
}

void Engine::Restore(context &ctx) {
	char StackEndsHere;
	while((ctx.Low <= &StackEndsHere) && (ctx.Hight >= &StackEndsHere)) {
		Restore(ctx);
	}
	std::memcpy(ctx.Low, std::get<0>(ctx.Stack), ctx.Hight - ctx.Low);
	longjmp(ctx.Environment, 1);
}

void Engine::Enter(context &ctx) {
	if ((cur_routine != nullptr) && (cur_routine != idle_ctx)) {
		if (setjmp(cur_routine->Environment) > 0) {
			return;
		}
		Store(*cur_routine); 
	}
	cur_routine = &ctx;
	Restore(ctx);
}

void Engine::yield() {
	context *next_ctx = this->alive;
	if (next_ctx != nullptr) {
		if (next_ctx == cur_routine) {
			next_ctx = next_ctx->next;
			if (next_ctx != nullptr) {
				Enter(*next_ctx);
			}
		} else {
			Enter(*next_ctx);
		}
	}

	if (cur_routine != nullptr && !cur_routine->blocked) {
		return;
	}
	if (cur_routine != idle_ctx) {
		Enter(*idle_ctx);
	}
	Enter(*idle_ctx);
}

void Engine::sched(void *routine_) {
	if (cur_routine == routine_) {
		return;
	} else if (routine_ != nullptr){
		Enter(*static_cast<context *>(routine_));
	} else {
		yield();
	}
}

void Engine::MoveCoroutine(context *&fromlist, context *&tolist, context *routine) {
    if (fromlist == tolist) {
        return;
    }
    if (routine->next != nullptr) {
        routine->next->prev = routine->prev;
    }
    if (routine->prev != nullptr) {
        routine->prev->next = routine->next;
    }
    if (routine == fromlist) {
        fromlist = fromlist->next;
    }
    routine->next = tolist;
    tolist = routine;
    if (routine->next != nullptr) {
        routine->next->prev = routine;
    }
}

void Engine::Block() {
    if (!(cur_routine->blocked)) {
        cur_routine->blocked = true;
        MoveCoroutine(alive, blocked, cur_routine);
    }
    yield();
}

void Engine::Wake(context *ctx) {
    if (ctx->blocked) {
        ctx->blocked = false;
        MoveCoroutine(blocked, alive, ctx);
    }
}

void Engine::WakeAll(void) {
    for (context *ctx = blocked; ctx != nullptr; ctx = blocked) {
        Wake(ctx);
    }
}

bool Engine::all_blocked() const { return !alive && blocked; }

Engine::context *Engine::get_cur_routine() const { return cur_routine; }


} // namespace Coroutine
} // namespace Afina
