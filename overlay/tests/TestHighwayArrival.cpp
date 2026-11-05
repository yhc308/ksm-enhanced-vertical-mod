#include <catch2/catch.hpp>
#include "MusicGame/Graphics/Highway/HighwayArrival.hpp"
#include "Vertical/VerticalLaneGeometry.hpp"
using namespace MusicGame;
using namespace MusicGame::Graphics;
namespace
{
	MeshData NeutralSource()
	{
		auto mesh = MeshData::Grid(kHighwayPlaneSizeWide, 1, 1, Float2{ .9925f, .995f }, Float2{ .00375f, .0025f });
		mesh.vertices[0].pos.z = mesh.vertices[1].pos.z = float(kHighwayPlaneSize.y * Vertical::kJudgeTextureT / 2);
		mesh.vertices[2].pos.z = mesh.vertices[3].pos.z = -kHighwayPlaneSize.y / 2;
		return mesh;
	}
}
TEST_CASE("Highway optical contact preserves upstream rather than forcing logical hit", "[Arrival]")
{
	const auto source = NeutralSource();
	const auto reference = HighwayArrival::UpstreamContactTextureT(source, {});
	REQUIRE(reference.has_value());
	REQUIRE(*reference > Vertical::kJudgeTextureT);
	REQUIRE(*reference < 1.0);
	for (const auto size : std::array<Size, 5>{ { { 640, 480 }, { 1280, 720 }, { 1920, 1080 }, { 720, 1280 }, { 1080, 1920 } } })
	{
		GameRender::Scope scope{ { size, true } };
		const auto camera = Vertical::MakeCamera(size, {});
		const auto span = Vertical::DisplayLaneSpan(source, camera, {}, size.y);
		REQUIRE(span.rayValid);
		REQUIRE(span.barTextureT == Approx(*reference).margin(1e-7));
		const auto display = Vertical::DisplayHighwayMesh(source, span);
		REQUIRE(display.vertices[2].tex.y == Approx(*reference).margin(1e-7));
	}
}
TEST_CASE("Display mesh updates reuse topology and do not move its outline", "[Arrival][Performance]")
{
	GameRender::Scope scope{ { { 720, 1280 }, true } };
	const auto source = NeutralSource();
	const auto camera = Vertical::MakeCamera({ 720, 1280 }, {});
	const auto span = Vertical::DisplayLaneSpan(source, camera, {}, 1280);
	auto mesh = Vertical::DisplayHighwayMesh(source, span);
	const auto original = mesh;
	auto* vertices = mesh.vertices.data();
	auto* indices = mesh.indices.data();
	for (int i = 0; i < 1000; ++i)
		Vertical::UpdateDisplayHighwayMesh(mesh, source, span);
	REQUIRE(mesh.vertices.data() == vertices);
	REQUIRE(mesh.indices.data() == indices);
	for (size_t i = 0; i < mesh.vertices.size(); ++i)
		REQUIRE(mesh.vertices[i].pos == original.vertices[i].pos);
	auto logical = span;
	logical.barTextureT = Vertical::kJudgeTextureT;
	const auto old = Vertical::DisplayHighwayMesh(source, logical);
	for (size_t i = 0; i < mesh.vertices.size(); ++i)
		REQUIRE(mesh.vertices[i].pos == old.vertices[i].pos);
}
TEST_CASE("Upstream ray mapping rejects degenerate and nonfinite poses", "[Arrival][Boundary]")
{
	using namespace HighwayArrival;
	REQUIRE_FALSE(RayParameter({ 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 0 }, { 0, 0, 10 }).has_value());
	REQUIRE_FALSE(RayParameter({ 0, 1, 0 }, { 0, 0, 1 }, { 0, 0, 0 }, { 0, 0, 0 }).has_value());
	const auto t = RayParameter({ 0, 10, -10 }, { 0, 2, 0 }, { 0, 0, 10 }, { 0, 0, -10 });
	REQUIRE(t.has_value());
	REQUIRE(*t == Approx(.375));
}

TEST_CASE("Independent artwork spins never drag the note mapping", "[Arrival][Rotation]")
{
	GameRender::Scope scope{ { { 720, 1280 }, true } };
	const auto source = NeutralSource();
	const auto camera = Vertical::MakeCamera({ 720, 1280 }, {});
	ViewStatus neutral;
	const auto reference = Vertical::DisplayLaneSpan(source, camera, neutral, 1280);
	for (const double laneSpin : { -180.0, -90.0, -10.0, 0.0, 10.0, 90.0, 180.0 })
	{
		for (const double barSpin : { -180.0, -90.0, -10.0, 0.0, 10.0, 90.0, 180.0 })
		{
			auto view = neutral;
			view.camStatus.rotationDegHighway = laneSpin;
			view.camStatus.rotationDegJdgline = barSpin;
			const auto span = Vertical::DisplayLaneSpan(source, camera, view, 1280);
			REQUIRE(span.rayValid);
			REQUIRE(span.anchorT == reference.anchorT);
			REQUIRE(span.endT == reference.endT);
			REQUIRE(span.barTextureT == reference.barTextureT);
		}
	}
}
