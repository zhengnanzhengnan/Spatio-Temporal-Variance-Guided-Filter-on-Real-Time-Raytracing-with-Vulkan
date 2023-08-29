
struct UniformBufferObject
{
	mat4 ModelView;//mv����
	mat4 Projection;//ͶӰ����
	mat4 ModelViewInverse;//mv�����
	mat4 ProjectionInverse;//ͶӰ�����

	float Aperture;//�����Ȧ��С������ģ�⾰��Ч������ȦԽ�󣬾���Ч��Խ���ԡ�
	float FocusDistance;//������࣬���ڼ��㾰��Ч����
	float HeatmapScale;//����ͼ�������ӣ����ڵ�������ͼ��ɫ��

	uint TotalNumberOfSamples;//�ܲ������������ڼ������ص�������ɫ��
	uint NumberOfSamples;//��ǰ�������������ڿ��Ʋ����Ľ��ȡ�
	uint NumberOfBounces;//���ߵķ������������ڿ��ƹ���׷�ٵ���ȡ�
	uint RandomSeed; //������ӣ����������������

	bool HasSky;//�Ƿ�����գ������ڹ���׷��ʱ�ж��Ƿ��������ɫ��
	bool ShowHeatmap;//�Ƿ���ʾ��ͼ�����ڵ��Ժ��Ż�����׷�ٵ����ܡ�����ͼ����ÿ�����ص���Ⱦʱ����ɫ����Ⱦʱ��Խ����������ɫԽ��
	uint FrameCounter;//֡������

	mat4 LastFrameModelView;//��һ֡mv�������ڷ�ͶӰ����motion vector
	mat4 LastFrameProjection;//��һ֡ͶӰ�������ڷ�ͶӰ����motion vector
};
