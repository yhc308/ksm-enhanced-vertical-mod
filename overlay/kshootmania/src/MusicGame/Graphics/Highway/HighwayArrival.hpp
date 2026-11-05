#pragma once
#include "MusicGame/Graphics/GraphicsDefines.hpp"
#include "MusicGame/Camera/CameraMath.hpp"
#include "MusicGame/ViewStatus.hpp"

namespace MusicGame::Graphics::HighwayArrival
{
	inline constexpr Vec3 kUpstreamCameraEye{ 0.0, 45.0, -366.0 };
	inline constexpr Float3 kUpstreamBarCenter{ 0.0f, 2.6f, -kHighwayPlaneSize.y / 2 - 0.4f };

	// Parameter of the ray/plane intersection in the untransformed highway.
	// The source plane is invariant in X; preserve doubles until vertex upload.
	inline Optional<double> RayParameter(const Vec3& eye, const Vec3& target, const Vec3& farPoint, const Vec3& nearPoint)
	{
		const double ry = target.y - eye.y, rz = target.z - eye.z;
		const double dy = nearPoint.y - farPoint.y, dz = nearPoint.z - farPoint.z;
		const double determinant = ry * dz - rz * dy;
		if (Abs(determinant) < 1e-5)
			return none;
		const double value = ((farPoint.y - eye.y) * rz - (farPoint.z - eye.z) * ry) / determinant;
		return std::isfinite(value) && Abs(value) <= 10000 ? Optional<double>{ value } : none;
	}

	// Preserve upstream's optical contact row, not its logical judgment row.
	// This is geometry/UV conversion, independent of BPM, speed and game clocks.
	inline Optional<double> UpstreamContactTextureT(const MeshData& source, const ViewStatus& view)
	{
		const auto& cam = view.camStatus;
		const double lineAngle = Math::ToRadians(cam.rotationDeg + cam.rotationDegJdgline) + view.tiltRadians;
		const double laneAngle = Math::ToRadians(cam.rotationDeg + cam.rotationDegHighway) + view.tiltRadians;
		const double overlayScale = cam.useLegacyJdgScale ? Camera::LegacyJdgoverlayScale(cam.zoomBottom) : Camera::JdgoverlayScale(cam.zoomBottom);
		const double lineScale = cam.useLegacyJdgScale ? Camera::LegacyJdglineScale(cam.zoomBottom) : Camera::JdglineScale(cam.zoomBottom);
		const auto line = Mat4x4::Scale(lineScale) * Mat4x4::Translate(Vec3::Right(cam.shiftX * overlayScale)) *
						  JudgmentPlaneTransformMatrix(lineAngle) * Mat4x4::Translate(kUpstreamBarCenter);
		const auto inverse = (Camera::CamShiftXMatrix(cam.shiftX) * TiltTransformMatrix(laneAngle)).inverse();
		const auto eye = inverse.transformPoint(Float3{ kUpstreamCameraEye });
		const auto bar = inverse.transformPoint(line.transformPoint(Float3::Zero()));
		const auto t = RayParameter(Vec3{ eye }, Vec3{ bar }, Vec3{ source.vertices[0].pos }, Vec3{ source.vertices[2].pos });
		if (!t)
			return none;
		const double uv = source.vertices[0].tex.y + (source.vertices[2].tex.y - source.vertices[0].tex.y) * *t;
		return std::isfinite(uv) && uv > 0 && uv < 1 ? Optional<double>{ uv } : none;
	}
}
