#pragma once

#include "AutoTrampoline.h"

#include "RE/B/BShkbAnimationGraph.h"
#include "RE/H/hkbClipGenerator.h"
#include "RE/H/hkbContext.h"

#include "MotionDataContainer.h"

namespace AMR
{
	class hkbClipGenerator
	{
#if BUILD_AE
		static constinit inline REL::ID ActivateId{ 59252 };
		static constinit inline REL::ID DeactivateId{ 59254 };
#elif BUILD_SE || BUILD_VR
		static constinit inline REL::ID ActivateId{ 58602 };
		static constinit inline REL::ID DeactivateId{ 58604 };
#endif
	public:

		static inline REL::Relocation<std::uintptr_t> Activate{ ActivateId };
		static inline REL::Relocation<std::uint32_t (*)(const RE::hkbClipGenerator*)> ComputeStartTime;
		static std::uint32_t ComputeStartTime_Hook(const RE::hkbClipGenerator* a_this, const RE::hkbContext* a_context);

		static inline REL::Relocation<std::uintptr_t> Deactivate{ DeactivateId };
		static inline REL::Relocation<void (*)(const RE::hkbClipGenerator*)> ResetIgnoreStartTime;
		static void ResetIgnoreStartTime_Hook(const RE::hkbClipGenerator* a_this, const RE::hkbContext* a_hkbContext);

	private:

		static const RE::hkaAnimation* GetBoundAnimation(const RE::hkbClipGenerator* a_this) 
		{ 
			return a_this->binding? (a_this->binding->animation? a_this->binding->animation.get() : nullptr) : nullptr;
		}
	};

	class MotionDataContainer
	{
#if BUILD_AE
		static constinit inline REL::ID ProcessTranslationDataId{ 32582 };
		static constinit inline REL::ID ProcessRotationDataId{ 32583 };
		static constinit inline REL::ID InterpolateRotationId{ 70836 };
#elif BUILD_SE || BUILD_VR
		static constinit inline REL::ID ProcessTranslationDataId{ 31812 };
		static constinit inline REL::ID ProcessRotationDataId{ 31813 };
		static constinit inline REL::ID InterpolateRotationId{ 69459 };
#endif
	public:

		// Translation
		static inline REL::Relocation<void (*)(std::uintptr_t*, float, RE::NiPoint3&)> ProcessTranslationData{ ProcessTranslationDataId };	// Called from unk_4DD5A0
		static void ProcessTranslationData_Hook(RE::MotionDataContainer* a_this, float a_motionTime, RE::NiPoint3& a_translation, const RE::BSFixedString* a_clipName);

		// Rotation
		static inline REL::Relocation<void (*)(std::uintptr_t*, float, RE::NiQuaternion&)> ProcessRotationData{ ProcessRotationDataId };								   // Called from unk_4DD5F0
		static inline REL::Relocation<void (*)(RE::NiQuaternion&, float, const RE::NiQuaternion&, const RE::NiQuaternion&)> InterpolateRotation{ InterpolateRotationId };  // Called from ProcessRotationData
		static void ProcessRotationData_Hook(RE::MotionDataContainer* a_this, float a_motionTime, RE::NiQuaternion& a_rotation, const RE::BSFixedString* a_clipName);

	private:

		static RE::hkbCharacter* GethkbCharacter(RE::Character* a_character)
		{
			RE::BSAnimationGraphManagerPtr animGraph;

			a_character->GetAnimationGraphManager(animGraph);

			if (animGraph && animGraph->graphs[animGraph->activeGraph])
			{
				return &animGraph->graphs[animGraph->activeGraph]->characterInstance;
			}

			return nullptr;
		};
	};

	class Character
	{
#if BUILD_AE
		static constinit inline REL::ID ProcessMotionDataId{ 32703 };
#elif BUILD_SE || BUILD_VR
		static constinit inline REL::ID ProcessMotionDataId{ 31949 };
#endif
	public:

		static inline REL::Relocation<std::uintptr_t> ProcessMotionData{ ProcessMotionDataId };
	};

	static inline void InstallHooks()
	{
		// As Bethesda used Havok libraries already compiled, neither registers 
		// nor offsets changed between SE-AE

		// hkbClipGenerator::Activate
		{
			struct Hook : Xbyak::CodeGenerator
			{
				Hook()
				{
					Xbyak::Label hookLabel;

					mov(rdx, r15);	// r15 = hkbContext*
					jmp(ptr[rip + hookLabel]);

					L(hookLabel);
					dq(reinterpret_cast<std::uintptr_t>(&hkbClipGenerator::ComputeStartTime_Hook));
				}
			};

			hkbClipGenerator::ComputeStartTime = SKSE::AutoTrampoline<5>(hkbClipGenerator::Activate.address() + 0x66E, Hook{});
		}

		// hkbClipGenerator::Deactivate
		{
			struct Hook : Xbyak::CodeGenerator
			{
				Hook()
				{
					Xbyak::Label hookLabel;

					mov(rdx, r14);	// r14 = hkbContext*
					jmp(ptr[rip + hookLabel]);

					L(hookLabel);
					dq(reinterpret_cast<std::uintptr_t>(&hkbClipGenerator::ResetIgnoreStartTime_Hook));
				}
			};

			hkbClipGenerator::ResetIgnoreStartTime = SKSE::AutoTrampoline<5>(hkbClipGenerator::Deactivate.address() + 0x1A, Hook{});
		}

		// Character::ProcessMotionData
		{
#if BUILD_AE
			static constinit std::uint64_t translation1HookOffset = 0x298;
			static constinit std::uint64_t translation2HookOffset = 0x2AA;
			static constinit std::uint64_t rotation1HookOffset = 0x35C;
			static constinit std::uint64_t rotation2HookOffset = 0x36D;
#elif BUILD_SE || BUILD_VR
			static constinit std::uint64_t translation1HookOffset = 0x28D;
			static constinit std::uint64_t translation2HookOffset = 0x2A1;
			static constinit std::uint64_t rotation1HookOffset = 0x355;
			static constinit std::uint64_t rotation2HookOffset = 0x368;
#endif
			struct Hook : Xbyak::CodeGenerator
			{
				Hook(void* a_func)
				{
					Xbyak::Label hookLabel;

					mov(r9, rbx);  // rbx = BSFixedString*
					jmp(ptr[rip + hookLabel]);

					L(hookLabel);
					dq(reinterpret_cast<std::uintptr_t>(a_func));
				}
			};

			// Translation
			SKSE::AutoTrampoline<5>(Character::ProcessMotionData.address() + translation1HookOffset,
									Hook{ &MotionDataContainer::ProcessTranslationData_Hook });
			SKSE::AutoTrampoline<5>(Character::ProcessMotionData.address() + translation2HookOffset,
									Hook{ &MotionDataContainer::ProcessTranslationData_Hook });

			// Rotation
			SKSE::AutoTrampoline<5>(Character::ProcessMotionData.address() + rotation1HookOffset,
									Hook{ &MotionDataContainer::ProcessRotationData_Hook });
			SKSE::AutoTrampoline<5>(Character::ProcessMotionData.address() + rotation2HookOffset,
									Hook{ &MotionDataContainer::ProcessRotationData_Hook });
		}
	}
}
