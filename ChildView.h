
// ChildView.h: CChildView 클래스의 인터페이스
//


#pragma once
#include <vector>
#include <atlimage.h>

struct Node {
	CPoint pt;
	int id;
};

struct Edge {
	int u, v;
	double dist;
};

// CChildView 창

class CChildView : public CWnd
{
// 생성입니다.
public:
	CChildView();

// 특성입니다.
public:
	CImage m_image;            // 지도 이미지
	std::vector<Node> m_nodes; // 생성된 점들
	std::vector<Edge> m_edges; // 생성된 선들
	std::vector<int> m_shortestPath; // 다익스트라 결과 경로

	int m_selectedNodeEdge;
	int m_selectedNodePath;

	int GetClickedNodeIndex(CPoint pt); // 클릭한 위치에 노드가 있는지 확인
	int GetClickedEdgeIndex(CPoint pt); // 클릭한 위치에 선이 있는지 확인
	CPoint ImagetoScreen(CPoint ptImage); // 이미지 좌표를 화면 좌표로 변환
	CPoint ScreentoImage(CPoint ptScreen); // 화면 좌표를 이미지 좌표로 변환
	void RunDijkstra(int startNode, int endNode); // 최단거리 계산

// 작업입니다.
public:

// 재정의입니다.
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// 구현입니다.
public:
	virtual ~CChildView();

	// 생성된 메시지 맵 함수
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

