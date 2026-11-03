#include "PLGLayout.h"

#include "Math/RandomStream.h"

namespace PLG
{
	namespace
	{
		constexpr int32 NumRoles = 5;
		constexpr float MatchTolerance = 1.f;

		template <typename T>
		void Shuffle(TArray<T>& Array, FRandomStream& Rng)
		{
			for (int32 i = Array.Num() - 1; i > 0; --i)
			{
				Array.Swap(i, Rng.RandRange(0, i));
			}
		}

		/** Weighted order: heavier modules tend to come first, everything still gets a turn. */
		TArray<int32> OrderByWeight(const TArray<int32>& Candidates, const TArray<FPLGModuleDef>& Defs, FRandomStream& Rng)
		{
			// Efraimidis-Spirakis: key = u^(1/w), sorted descending.
			TArray<TPair<float, int32>> Keyed;
			for (int32 Index : Candidates)
			{
				const float Weight = FMath::Max(Defs[Index].Weight, KINDA_SMALL_NUMBER);
				Keyed.Emplace(FMath::Pow(FMath::Max(Rng.FRand(), KINDA_SMALL_NUMBER), 1.f / Weight), Index);
			}
			Keyed.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key > B.Key; });
			TArray<int32> Out;
			for (const TPair<float, int32>& Pair : Keyed)
			{
				Out.Add(Pair.Value);
			}
			return Out;
		}

		/** One generation attempt. */
		class FBuilder
		{
		public:
			FBuilder(const FPLGLayoutSettings& InSettings, int32 InSeed, FPLGLayout& InLayout)
				: Settings(InSettings), Defs(InSettings.Modules), Rng(InSeed), Layout(InLayout)
			{
				for (int32 i = 0; i < Defs.Num(); ++i)
				{
					ByRole[int32(Defs[i].Role)].Add(i);
				}
			}

			bool Run(FString& OutFailure)
			{
				Layout.Modules.Reset();
				Open.Reset();

				if (!PlaceStart())
				{
					OutFailure = TEXT("no Start module");
					return false;
				}
				const TArray<FIntPoint> StartDoors = Open;
				if (!PlaceRoleAtAny(EPLGRole::Hub, StartDoors))
				{
					OutFailure = TEXT("the hub does not fit on any start door");
					return false;
				}

				int32 RoomsLeft = Settings.RoomCount;
				for (int32 Step = 0; RoomsLeft > 0 && Step < Settings.MaxSteps && Open.Num() > 0; ++Step)
				{
					const FIntPoint At = Open[Rng.RandHelper(Open.Num())];
					const bool bCorridorFirst = Rng.FRand() < Settings.CorridorChance;
					const EPLGRole First = bCorridorFirst ? EPLGRole::Corridor : EPLGRole::Room;
					const EPLGRole Second = bCorridorFirst ? EPLGRole::Room : EPLGRole::Corridor;
					if (PlaceRoleAt(First, At))
					{
						RoomsLeft -= First == EPLGRole::Room ? 1 : 0;
					}
					else if (PlaceRoleAt(Second, At))
					{
						RoomsLeft -= Second == EPLGRole::Room ? 1 : 0;
					}
					else
					{
						Open.Remove(At); // nothing fits behind this door: it stays wall
					}
				}
				if (RoomsLeft > 0)
				{
					OutFailure = FString::Printf(TEXT("%d room(s) did not fit"), RoomsLeft);
					return false;
				}

				// The end room goes as deep as it fits.
				TArray<FIntPoint> Deepest = Open;
				Shuffle(Deepest, Rng);
				Deepest.StableSort([this](const FIntPoint& A, const FIntPoint& B) { return Layout.Modules[A.X].Depth > Layout.Modules[B.X].Depth; });
				if (!PlaceRoleAtAny(EPLGRole::End, Deepest))
				{
					OutFailure = TEXT("the end room does not fit on any open door");
					return false;
				}

				PruneDeadEndCorridors();
				AssignThemes();
				return true;
			}

		private:
			bool PlaceStart()
			{
				const TArray<int32> Starts = OrderByWeight(ByRole[int32(EPLGRole::Start)], Defs, Rng);
				if (Starts.Num() == 0)
				{
					return false;
				}
				FPLGPlacedModule Start;
				Start.Def = Starts[0];
				Start.Links.Init(INDEX_NONE, Defs[Start.Def].Sockets.Num());
				Layout.Modules.Add(Start);
				for (int32 S = 0; S < Defs[Start.Def].Sockets.Num(); ++S)
				{
					Open.Add(FIntPoint(0, S));
				}
				return true;
			}

			bool PlaceRoleAtAny(EPLGRole Role, const TArray<FIntPoint>& Doors)
			{
				for (const FIntPoint& At : Doors)
				{
					if (Open.Contains(At) && PlaceRoleAt(Role, At))
					{
						return true;
					}
				}
				return false;
			}

			bool PlaceRoleAt(EPLGRole Role, const FIntPoint& At)
			{
				for (int32 DefIndex : OrderByWeight(ByRole[int32(Role)], Defs, Rng))
				{
					if (TryPlace(DefIndex, At))
					{
						return true;
					}
				}
				return false;
			}

			/** Puts DefIndex behind the open door At, trying each of its matching sockets; false if every way overlaps. */
			bool TryPlace(int32 DefIndex, const FIntPoint& At)
			{
				// Copies: Layout.Modules may reallocate once the new module is added.
				const FPLGPlacedModule From = Layout.Modules[At.X];
				const FPLGSocket DoorA = Defs[From.Def].Sockets[At.Y];
				const FVector2D PointA = SocketWorldPoint(Defs[From.Def], From, DoorA);
				const int32 QuarterA = SocketWorldQuarter(From, DoorA);

				const FPLGModuleDef& Def = Defs[DefIndex];
				TArray<int32> Candidates;
				for (int32 S = 0; S < Def.Sockets.Num(); ++S)
				{
					if (FMath::IsNearlyEqual(Def.Sockets[S].Width, DoorA.Width))
					{
						Candidates.Add(S);
					}
				}
				Shuffle(Candidates, Rng);

				for (int32 SocketB : Candidates)
				{
					FPLGPlacedModule New;
					New.Def = DefIndex;
					// Socket B must face back at socket A.
					New.Quarter = ((QuarterA + 2 - int32(Def.Sockets[SocketB].Edge)) % 4 + 4) % 4;
					New.Location = PointA - Rotate(SocketLocalPoint(Def, Def.Sockets[SocketB]), New.Quarter);
					if (!Fits(Footprint(Def, New)))
					{
						continue;
					}
					New.Depth = From.Depth + 1;
					New.Links.Init(INDEX_NONE, Def.Sockets.Num());
					const int32 NewIndex = Layout.Modules.Add(New);
					Link(At.X, At.Y, NewIndex, SocketB);
					Open.Remove(At);

					// Its other doors either meet a door that is already open (closing a loop) or become open.
					for (int32 S = 0; S < Def.Sockets.Num(); ++S)
					{
						if (S == SocketB)
						{
							continue;
						}
						const FVector2D Point = SocketWorldPoint(Def, Layout.Modules[NewIndex], Def.Sockets[S]);
						const int32 Facing = (SocketWorldQuarter(Layout.Modules[NewIndex], Def.Sockets[S]) + 2) % 4;
						const int32 Match = Open.IndexOfByPredicate([&](const FIntPoint& Other)
						{
							const FPLGPlacedModule& OtherModule = Layout.Modules[Other.X];
							const FPLGSocket& OtherSocket = Defs[OtherModule.Def].Sockets[Other.Y];
							return FMath::IsNearlyEqual(OtherSocket.Width, Def.Sockets[S].Width)
								&& SocketWorldQuarter(OtherModule, OtherSocket) == Facing
								&& FVector2D::Distance(SocketWorldPoint(Defs[OtherModule.Def], OtherModule, OtherSocket), Point) < MatchTolerance;
						});
						if (Match != INDEX_NONE)
						{
							Link(Open[Match].X, Open[Match].Y, NewIndex, S);
							Open.RemoveAt(Match);
						}
						else
						{
							Open.Add(FIntPoint(NewIndex, S));
						}
					}
					return true;
				}
				return false;
			}

			bool Fits(const FBox2D& Box) const
			{
				const float Limit = Settings.MaxHalfExtent;
				if (Box.Min.X < -Limit || Box.Min.Y < -Limit || Box.Max.X > Limit || Box.Max.Y > Limit)
				{
					return false;
				}
				// Shrunk by a centimeter: modules may share an edge, never area.
				const FBox2D Inner = Box.ExpandBy(-MatchTolerance);
				for (const FPLGPlacedModule& Placed : Layout.Modules)
				{
					if (!Placed.bRemoved && Inner.Intersect(Footprint(Defs[Placed.Def], Placed).ExpandBy(-MatchTolerance)))
					{
						return false;
					}
				}
				return true;
			}

			void Link(int32 ModuleA, int32 SocketA, int32 ModuleB, int32 SocketB)
			{
				Layout.Modules[ModuleA].Links[SocketA] = ModuleB;
				Layout.Modules[ModuleB].Links[SocketB] = ModuleA;
			}

			/** A corridor that leads nowhere is just a closet: remove it, and the door to it becomes wall. */
			void PruneDeadEndCorridors()
			{
				bool bChanged = true;
				while (bChanged)
				{
					bChanged = false;
					for (int32 i = 0; i < Layout.Modules.Num(); ++i)
					{
						FPLGPlacedModule& Module = Layout.Modules[i];
						if (Module.bRemoved || Defs[Module.Def].Role != EPLGRole::Corridor)
						{
							continue;
						}
						int32 NumLinks = 0;
						for (int32 Other : Module.Links)
						{
							NumLinks += Other != INDEX_NONE ? 1 : 0;
						}
						if (NumLinks > 1)
						{
							continue;
						}
						for (int32& Other : Module.Links)
						{
							if (Other != INDEX_NONE)
							{
								for (int32& Back : Layout.Modules[Other].Links)
								{
									if (Back == i)
									{
										Back = INDEX_NONE;
									}
								}
								Other = INDEX_NONE;
							}
						}
						Module.bRemoved = true;
						bChanged = true;
					}
				}
			}

			/** Rooms walk through a shuffled theme list, so the first rooms never repeat a theme. */
			void AssignThemes()
			{
				if (Settings.NumThemes <= 0)
				{
					return;
				}
				TArray<int32> Order;
				for (int32 i = 0; i < Settings.NumThemes; ++i)
				{
					Order.Add(i);
				}
				Shuffle(Order, Rng);
				int32 Next = 0;
				for (FPLGPlacedModule& Module : Layout.Modules)
				{
					if (!Module.bRemoved && Defs[Module.Def].Role == EPLGRole::Room)
					{
						Module.Theme = Order[Next++ % Order.Num()];
					}
				}
			}

			const FPLGLayoutSettings& Settings;
			const TArray<FPLGModuleDef>& Defs;
			FRandomStream Rng;
			FPLGLayout& Layout;
			TArray<int32> ByRole[NumRoles];
			/** Doors not connected yet: (module, socket). */
			TArray<FIntPoint> Open;
		};
	}

	FVector2D Rotate(const FVector2D& V, int32 Quarter)
	{
		switch (((Quarter % 4) + 4) % 4)
		{
		case 1: return FVector2D(-V.Y, V.X);
		case 2: return FVector2D(-V.X, -V.Y);
		case 3: return FVector2D(V.Y, -V.X);
		default: return V;
		}
	}

	FVector2D SocketLocalPoint(const FPLGModuleDef& Def, const FPLGSocket& Socket)
	{
		const FVector2D Half = Def.Size * 0.5f;
		switch (Socket.Edge)
		{
		case EPLGEdge::PosX: return FVector2D(Half.X, Socket.Offset);
		case EPLGEdge::NegX: return FVector2D(-Half.X, Socket.Offset);
		case EPLGEdge::PosY: return FVector2D(Socket.Offset, Half.Y);
		default: return FVector2D(Socket.Offset, -Half.Y);
		}
	}

	FVector2D SocketWorldPoint(const FPLGModuleDef& Def, const FPLGPlacedModule& Placed, const FPLGSocket& Socket)
	{
		return Placed.Location + Rotate(SocketLocalPoint(Def, Socket), Placed.Quarter);
	}

	int32 SocketWorldQuarter(const FPLGPlacedModule& Placed, const FPLGSocket& Socket)
	{
		return (int32(Socket.Edge) + Placed.Quarter) % 4;
	}

	FBox2D Footprint(const FPLGModuleDef& Def, const FPLGPlacedModule& Placed)
	{
		const FVector2D Half = (Placed.Quarter % 2 == 0 ? Def.Size : FVector2D(Def.Size.Y, Def.Size.X)) * 0.5f;
		return FBox2D(Placed.Location - Half, Placed.Location + Half);
	}

	TArray<FString> ValidateModules(const TArray<FPLGModuleDef>& Modules, float Grid)
	{
		TArray<FString> Problems;
		auto OnGrid = [](float Value, float Step) { return FMath::IsNearlyZero(FMath::Fmod(Value, Step), 0.01f); };
		bool bRoles[NumRoles] = {};
		for (const FPLGModuleDef& Def : Modules)
		{
			bRoles[int32(Def.Role)] = true;
			// Twice the grid keeps the centered pivot on a grid point.
			if (!OnGrid(Def.Size.X, Grid * 2.f) || !OnGrid(Def.Size.Y, Grid * 2.f))
			{
				Problems.Add(FString::Printf(TEXT("%s: size %.0f x %.0f is not a multiple of %.0f cm"), *Def.Name.ToString(), Def.Size.X, Def.Size.Y, Grid * 2.f));
			}
			if (Def.Sockets.Num() == 0)
			{
				Problems.Add(FString::Printf(TEXT("%s: no door sockets"), *Def.Name.ToString()));
			}
			for (const FPLGSocket& Socket : Def.Sockets)
			{
				const bool bXEdge = Socket.Edge == EPLGEdge::PosX || Socket.Edge == EPLGEdge::NegX;
				const float EdgeHalf = (bXEdge ? Def.Size.Y : Def.Size.X) * 0.5f;
				if (!OnGrid(Socket.Offset, Grid))
				{
					Problems.Add(FString::Printf(TEXT("%s: socket offset %.0f is off the %.0f cm grid"), *Def.Name.ToString(), Socket.Offset, Grid));
				}
				// Leave a wall stub of at least one grid cell beside every door.
				if (FMath::Abs(Socket.Offset) + Socket.Width * 0.5f > EdgeHalf - Grid + 0.01f)
				{
					Problems.Add(FString::Printf(TEXT("%s: a %.0f cm door at offset %.0f is too close to the corner"), *Def.Name.ToString(), Socket.Width, Socket.Offset));
				}
			}
		}
		const TCHAR* RoleNames[NumRoles] = { TEXT("Start"), TEXT("Hub"), TEXT("End"), TEXT("Room"), TEXT("Corridor") };
		for (int32 Role = 0; Role < NumRoles; ++Role)
		{
			if (!bRoles[Role])
			{
				Problems.Add(FString::Printf(TEXT("no module with role %s"), RoleNames[Role]));
			}
		}
		return Problems;
	}

	bool GenerateLayout(const FPLGLayoutSettings& Settings, int32 Seed, FPLGLayout& OutLayout, FString& OutReport)
	{
		FString Failure;
		const int32 Attempts = FMath::Max(1, Settings.MaxAttempts);
		for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
		{
			const int32 AttemptSeed = Attempt == 0 ? Seed : int32(HashCombineFast(uint32(Seed), uint32(Attempt)));
			FBuilder Builder(Settings, AttemptSeed, OutLayout);
			if (Builder.Run(Failure))
			{
				OutLayout.Seed = Seed;
				int32 Count = 0;
				for (const FPLGPlacedModule& Module : OutLayout.Modules)
				{
					Count += Module.bRemoved ? 0 : 1;
				}
				OutReport = FString::Printf(TEXT("seed %d: %d modules (attempt %d)"), Seed, Count, Attempt + 1);
				return true;
			}
		}
		OutLayout.Modules.Reset();
		OutReport = FString::Printf(TEXT("seed %d: failed after %d attempts, last: %s"), Seed, Attempts, *Failure);
		return false;
	}

	TArray<FPLGModuleDef> MakeSampleModules()
	{
		constexpr float Small = 150.f;
		constexpr float Double = 300.f;
		auto Door = [](EPLGEdge Edge, float Width, float Offset = 0.f)
		{
			FPLGSocket Socket;
			Socket.Edge = Edge;
			Socket.Width = Width;
			Socket.Offset = Offset;
			return Socket;
		};
		auto Module = [](const TCHAR* Name, EPLGRole Role, float X, float Y, TArray<FPLGSocket> Sockets, float Weight = 1.f)
		{
			FPLGModuleDef Def;
			Def.Name = Name;
			Def.Role = Role;
			Def.Size = FVector2D(X, Y);
			Def.Sockets = MoveTemp(Sockets);
			Def.Weight = Weight;
			return Def;
		};

		TArray<FPLGModuleDef> Out;
		// Fixed rooms
		Out.Add(Module(TEXT("Start"), EPLGRole::Start, 1000.f, 1000.f, { Door(EPLGEdge::PosX, Double) }));
		Out.Add(Module(TEXT("Hub"), EPLGRole::Hub, 1600.f, 1600.f,
			{ Door(EPLGEdge::NegX, Double), Door(EPLGEdge::PosX, Double), Door(EPLGEdge::PosY, Small), Door(EPLGEdge::NegY, Small) }));
		Out.Add(Module(TEXT("End"), EPLGRole::End, 800.f, 800.f, { Door(EPLGEdge::NegX, Small) }));
		// General room frames, reused and themed
		Out.Add(Module(TEXT("RoomSmall"), EPLGRole::Room, 800.f, 800.f,
			{ Door(EPLGEdge::NegX, Small), Door(EPLGEdge::PosX, Small), Door(EPLGEdge::PosY, Small) }));
		Out.Add(Module(TEXT("RoomLarge"), EPLGRole::Room, 1200.f, 1000.f,
			{ Door(EPLGEdge::NegX, Double), Door(EPLGEdge::PosX, Small), Door(EPLGEdge::PosY, Small, 300.f), Door(EPLGEdge::NegY, Small, -300.f) }));
		// Corridors, 300 cm wide
		Out.Add(Module(TEXT("CorridorStraight"), EPLGRole::Corridor, 800.f, 300.f, { Door(EPLGEdge::NegX, Small), Door(EPLGEdge::PosX, Small) }, 2.f));
		Out.Add(Module(TEXT("CorridorCorner"), EPLGRole::Corridor, 300.f, 300.f, { Door(EPLGEdge::NegX, Small), Door(EPLGEdge::PosY, Small) }));
		Out.Add(Module(TEXT("CorridorT"), EPLGRole::Corridor, 300.f, 300.f,
			{ Door(EPLGEdge::NegX, Small), Door(EPLGEdge::PosX, Small), Door(EPLGEdge::PosY, Small) }));
		return Out;
	}

	TArray<FPLGTheme> MakeSampleThemes()
	{
		auto Theme = [](const TCHAR* Name, const FLinearColor& Floor, const FLinearColor& Wall)
		{
			FPLGTheme Out;
			Out.Name = Name;
			Out.FloorColor = Floor;
			Out.WallColor = Wall;
			return Out;
		};
		return {
			Theme(TEXT("Theme1"), FLinearColor(0.75f, 0.55f, 0.6f), FLinearColor(0.98f, 0.88f, 0.9f)),
			Theme(TEXT("Theme2"), FLinearColor(0.45f, 0.6f, 0.75f), FLinearColor(0.85f, 0.93f, 1.f)),
			Theme(TEXT("Theme3"), FLinearColor(0.45f, 0.32f, 0.2f), FLinearColor(0.8f, 0.72f, 0.55f)),
			Theme(TEXT("Theme4"), FLinearColor(0.2f, 0.18f, 0.25f), FLinearColor(0.45f, 0.4f, 0.5f)),
		};
	}
}
