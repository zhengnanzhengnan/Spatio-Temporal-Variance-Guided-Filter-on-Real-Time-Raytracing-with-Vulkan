
struct UniformBufferObject
{
	mat4 ModelView;//mv矩阵
	mat4 Projection;//投影矩阵
	mat4 ModelViewInverse;//mv逆矩阵
	mat4 ProjectionInverse;//投影逆矩阵

	float Aperture;//相机光圈大小，用于模拟景深效果。光圈越大，景深效果越明显。
	float FocusDistance;//相机焦距，用于计算景深效果。
	float HeatmapScale;//热力图缩放因子，用于调整热力图颜色。

	uint TotalNumberOfSamples;//总采样次数，用于计算像素的最终颜色。
	uint NumberOfSamples;//当前采样次数，用于控制采样的进度。
	uint NumberOfBounces;//光线的反弹次数，用于控制光线追踪的深度。
	uint RandomSeed; //随机种子，用于生成随机数。

	bool HasSky;//是否有天空，用于在光线追踪时判断是否有天空颜色。
	bool ShowHeatmap;//是否显示热图，用于调试和优化光线追踪的性能。热力图根据每个像素的渲染时间着色，渲染时间越长的像素颜色越深
	uint FrameCounter;//帧计数器

	mat4 LastFrameModelView;//上一帧mv矩阵，用于反投影计算motion vector
	mat4 LastFrameProjection;//上一帧投影矩阵，用于反投影计算motion vector
};
