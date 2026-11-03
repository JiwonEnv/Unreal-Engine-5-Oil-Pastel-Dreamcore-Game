// Oil pastel / colored pencil post-process. Body of the Custom node in M_PP_OilPastel (after tonemapping).
// build_shader.py pastes this file into the node: edit here and rerun it, or edit the node directly.
//
// Passes, all in one shader:
//   1. Kuwahara filter         - flattens color into brush-like patches (oil paint look)
//   2. Pigment on paper tooth  - procedural paper grain + hatching that follows surface direction;
//                                dark colors press harder and fill the grain, light colors leave paper showing
//   3. Pencil outlines         - depth and normal edges, slightly wobbly, with optional "line boil"
//   4. Star glints             - 4-point stars on bright, glossy pixels (glass, brass)
//   5. Dream collapse          - fades the color away until only the pencil sketch remains
//
// Inputs (wired by build_shader.py):
//   SceneColor, SceneDepthIn, WorldNormalIn, RoughnessIn: SceneTexture nodes. Unused here, but connecting them
//     is what makes the material compiler bind those buffers for SceneTextureLookup.
//   BrushSize, PaperScale, Grain, StrokeScale, Stroke, Saturation, LineWidth, LineStrength,
//   GlintSize, Glint, GlintThreshold, Collapse, BoilFPS, Time: scalars
//   PaperColor, InkColor, GlintColor: float3

struct FOilPastel
{
	float2 ViewSize;
	float2 Texel;

	float Hash(float2 P)
	{
		P = frac(P * float2(123.34, 456.21));
		P += dot(P, P + 45.32);
		return frac(P.x * P.y);
	}

	float Noise(float2 P)
	{
		float2 I = floor(P);
		float2 F = frac(P);
		float2 U = F * F * (3.0 - 2.0 * F);
		return lerp(lerp(Hash(I), Hash(I + float2(1, 0)), U.x),
		            lerp(Hash(I + float2(0, 1)), Hash(I + float2(1, 1)), U.x), U.y);
	}

	float Fbm(float2 P)
	{
		float Sum = 0.0;
		float Amp = 0.5;
		for (int Octave = 0; Octave < 4; Octave++)
		{
			Sum += Amp * Noise(P);
			P = P * 2.03 + 17.1;
			Amp *= 0.5;
		}
		return Sum;
	}

	float Luma(float3 C) { return dot(C, float3(0.299, 0.587, 0.114)); }

	float4 Fetch(float2 ViewportUV, int Id)
	{
		return SceneTextureLookup(ViewportUVToSceneTextureUV(saturate(ViewportUV), Id), Id, false);
	}

	float3 Color(float2 UV)     { return Fetch(UV, PPI_PostProcessInput0).rgb; }
	float  Depth(float2 UV)     { return Fetch(UV, PPI_SceneDepth).r; }
	float3 Normal(float2 UV)    { return Fetch(UV, PPI_WorldNormal).rgb; }
	float  Roughness(float2 UV) { return Fetch(UV, PPI_Roughness).r; }

	// 1. Classic 4-quadrant Kuwahara on a fixed 9x9 grid, scaled to BrushSize pixels.
	float3 Kuwahara(float2 UV, float BrushSize)
	{
		const int R = 4;
		float2 Step = Texel * max(BrushSize, 1.0) / R;
		float3 Mean[4] = { (float3)0, (float3)0, (float3)0, (float3)0 };
		float3 Sq[4]   = { (float3)0, (float3)0, (float3)0, (float3)0 };

		for (int Y = -R; Y <= R; Y++)
		{
			for (int X = -R; X <= R; X++)
			{
				float3 C = Color(UV + float2(X, Y) * Step);
				float3 C2 = C * C;
				// Pixels on the axes belong to both neighbouring quadrants.
				if (X <= 0 && Y <= 0) { Mean[0] += C; Sq[0] += C2; }
				if (X >= 0 && Y <= 0) { Mean[1] += C; Sq[1] += C2; }
				if (X <= 0 && Y >= 0) { Mean[2] += C; Sq[2] += C2; }
				if (X >= 0 && Y >= 0) { Mean[3] += C; Sq[3] += C2; }
			}
		}

		const float N = (R + 1) * (R + 1);
		float Best = 1e9;
		float3 Result = Mean[0] / N;
		for (int Q = 0; Q < 4; Q++)
		{
			float3 M = Mean[Q] / N;
			float3 V = abs(Sq[Q] / N - M * M);
			float Var = V.r + V.g + V.b;
			if (Var < Best)
			{
				Best = Var;
				Result = M;
			}
		}
		return Result;
	}

	// Screen-space stroke direction: along the surface, i.e. perpendicular to the projected normal.
	// Surfaces facing the camera get the classic 45-degree hatch.
	float2 StrokeDirection(float3 WorldN)
	{
		float3 ViewN = mul(WorldN, (float3x3)ResolvedView.TranslatedWorldToView);
		float2 ScreenN = float2(ViewN.x, -ViewN.y);
		float Len = length(ScreenN);
		float2 Along = Len > 1e-3 ? float2(-ScreenN.y, ScreenN.x) / Len : float2(0.7071, 0.7071);
		return normalize(lerp(float2(0.7071, 0.7071), Along, saturate(Len * 2.0)));
	}

	// 3. Pencil edges from depth (silhouettes) and normals (creases).
	float Outline(float2 UV, float2 Pixel, float D0, float3 N0, float LineWidth, float Seed)
	{
		if (D0 > 100000.0)
		{
			return 0.0; // sky
		}
		// Hand-drawn wobble: offset the probe by low-frequency noise, re-rolled BoilFPS times a second.
		float2 Wobble = (float2(Noise(Pixel * 0.03 + Seed), Noise(Pixel * 0.03 + Seed + 31.7)) - 0.5) * LineWidth * 1.2;
		float2 Base = UV + Wobble * Texel;
		float2 Offsets[4] = { float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1) };

		float DepthEdge = 0.0;
		float NormalEdge = 0.0;
		for (int K = 0; K < 4; K++)
		{
			float2 P = Base + Offsets[K] * LineWidth * Texel;
			float D = Depth(P);
			DepthEdge = max(DepthEdge, abs(D - D0) / max(D0, 1.0));
			NormalEdge = max(NormalEdge, 1.0 - saturate(dot(Normal(P), N0)));
		}
		// Far away, only big silhouettes survive, like a sketch losing detail in the distance.
		float Distance = saturate(D0 / 6000.0);
		float Line = max(smoothstep(0.04 + Distance * 0.1, 0.12 + Distance * 0.2, DepthEdge),
		                 smoothstep(0.25, 0.6, NormalEdge) * (1.0 - Distance));
		return Line;
	}

	// 4. Gather-style star: a pixel lights up when a glossy highlight sits along its horizontal or vertical arm.
	float Glints(float2 UV, float GlintSize, float Threshold)
	{
		float2 Dirs[4] = { float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1) };
		const int Steps = 6;
		float Star = 0.0;
		for (int K = 0; K <= Steps; K++)
		{
			float T = K / (float)Steps;
			float Falloff = (1.0 - T) * (1.0 - T);
			for (int A = 0; A < (K == 0 ? 1 : 4); A++)
			{
				float2 P = UV + Dirs[A] * T * GlintSize * Texel;
				float Bright = saturate((Luma(Color(P)) - Threshold) / max(1.0 - Threshold, 1e-3));
				float Glossy = saturate(1.0 - Roughness(P) * 2.5);
				Star = max(Star, Bright * Glossy * Falloff);
			}
		}
		return Star;
	}

	float3 Run(float2 UV, float BrushSize, float PaperScale, float Grain, float StrokeScale, float Stroke,
	           float Saturation, float LineWidth, float LineStrength, float GlintSize, float Glint,
	           float GlintThreshold, float Collapse, float BoilFPS, float Time,
	           float3 PaperColor, float3 InkColor, float3 GlintColor)
	{
		float2 Pixel = UV * ViewSize;
		float Seed = BoilFPS > 0.0 ? floor(Time * BoilFPS) * 7.13 : 0.0;

		float D0 = Depth(UV);
		float3 N0 = Normal(UV);
		bool bSky = D0 > 100000.0;

		// 1. Paint
		float3 Paint = Kuwahara(UV, BrushSize);
		float L = Luma(Paint);
		Paint = max(lerp(L.xxx, Paint, Saturation * (1.0 - Collapse)), 0.0);

		// 2. Pigment on paper tooth
		float Tooth = Fbm(Pixel / max(PaperScale, 0.1));
		float2 Along = bSky ? float2(0.7071, 0.7071) : StrokeDirection(N0);
		float2 Across = float2(-Along.y, Along.x);
		float2 StrokeUV = float2(dot(Pixel, Along) / (StrokeScale * 8.0), dot(Pixel, Across) / StrokeScale);
		// Hatching ignores the boil seed: only the outlines jitter, the paper and strokes stay put.
		float Hatch = Noise(StrokeUV);
		float Surface = lerp(Tooth, Hatch, Stroke);

		// Dark pigment is pressed hard enough to fill the grain; light pigment only catches the peaks.
		float Pressure = saturate(0.45 + (1.0 - L) * 0.9);
		float Coverage = smoothstep(-0.1, 0.25, Pressure - Surface * Grain);
		Coverage *= 1.0 - Collapse;
		// Waxy build-up: overlapping strokes are slightly denser.
		float3 Pigment = Paint * lerp(1.0, 0.85 + 0.3 * Hatch, Stroke * 0.5);
		float3 Result = lerp(PaperColor, Pigment, Coverage);

		// 5. Collapse: soft graphite shading replaces the color, following the same hatch.
		float Graphite = saturate((1.0 - L) * 0.8 - Surface * 0.35) * Collapse;
		Result = lerp(Result, InkColor, Graphite * 0.6);

		// 3. Outlines, with paper grain breaking up the line like a real pencil.
		float Line = Outline(UV, Pixel, D0, N0, LineWidth, Seed) * LineStrength;
		Line *= 0.65 + 0.35 * Tooth;
		Line = saturate(Line * (1.0 + Collapse * 0.5));
		Result = lerp(Result, InkColor, Line);

		// 4. Glints (gone once the dream collapses)
		if (Glint > 0.0 && !bSky)
		{
			Result = lerp(Result, GlintColor, saturate(Glints(UV, GlintSize, GlintThreshold) * Glint * (1.0 - Collapse)));
		}

		// Paper darkens slightly toward the edges of the sheet.
		float2 Center = UV - 0.5;
		Result *= lerp(1.0, 0.86, pow(saturate(length(Center) * 1.25), 3.0));
		return Result;
	}
};

FOilPastel Pastel;
Pastel.ViewSize = ResolvedView.ViewSizeAndInvSize.xy;
Pastel.Texel = ResolvedView.ViewSizeAndInvSize.zw;
float2 ViewportUV = GetViewportUV(Parameters);
return Pastel.Run(ViewportUV, BrushSize, PaperScale, Grain, StrokeScale, Stroke, Saturation, LineWidth, LineStrength,
	GlintSize, Glint, GlintThreshold, Collapse, BoilFPS, Time, PaperColor, InkColor, GlintColor);
