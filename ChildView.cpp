
// ChildView.cpp: CChildView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
#include "school.h"
#include "ChildView.h"
#include <cmath>
#include <queue>
#include <limits>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

const int NODE_RADIUS = 8;

// CChildView

CChildView::CChildView()
{
	m_selectedNodeEdge = -1;
	m_selectedNodePath = -1;

	HRESULT hResult = m_image.Load(_T("school_map.png"));
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// CChildView 메시지 처리기

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); 
	CRect rect;
	GetClientRect(&rect);

	// 1. Draw the background image
	if (!m_image.IsNull())
	{
		::SetStretchBltMode(dc.m_hDC, HALFTONE);
		m_image.Draw(dc.m_hDC, 0, 0, rect.Width(), rect.Height());
	}

	// 2. Draw edges
	CPen penEdge(PS_SOLID, 2, RGB(0, 0, 255)); // Blue pen for edges
	CPen* pOldPen = dc.SelectObject(&penEdge);

	for (const auto& edge : m_edges)
	{
		CPoint p1 = ImagetoScreen(m_nodes[edge.u].pt);
		CPoint p2 = ImagetoScreen(m_nodes[edge.v].pt);
		dc.MoveTo(p1);
		dc.LineTo(p2);
	}
	dc.SelectObject(pOldPen);

	// 3. Highlight shortest path
	if (m_shortestPath.size() > 1)
	{
		CPen penPath(PS_SOLID, 4, RGB(255, 0, 0)); // Red pen for shortest path
		dc.SelectObject(&penPath);
		
		for (size_t i = 0; i < m_shortestPath.size() - 1; ++i)
		{
			CPoint p1 = ImagetoScreen(m_nodes[m_shortestPath[i]].pt);
			CPoint p2 = ImagetoScreen(m_nodes[m_shortestPath[i + 1]].pt);
			dc.MoveTo(p1);
			dc.LineTo(p2);
		}
		dc.SelectObject(pOldPen);
	}

	// 4. Draw nodes
	CBrush brushNormal(RGB(0, 0, 0)); // Black brush for nodes
	CBrush brushSelected(RGB(255, 255, 0)); // Yellow brush for selected node
	CBrush* pOldBrush = NULL;

	for (int i = 0; i < m_nodes.size(); ++i)
	{
		bool isSelected = (i == m_selectedNodeEdge || i == m_selectedNodePath);
		pOldBrush = dc.SelectObject(isSelected ? &brushSelected : &brushNormal);

		CPoint screenPt = ImagetoScreen(m_nodes[i].pt);

		CRect r(screenPt.x - NODE_RADIUS, screenPt.y - NODE_RADIUS,
			screenPt.x + NODE_RADIUS, screenPt.y + NODE_RADIUS);
		dc.Ellipse(r);

		dc.SelectObject(pOldBrush);
	}
}


void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// Check if keyboard is clicked
	bool bCtrl = (GetKeyState(VK_CONTROL) < 0);
	bool bShift = (GetKeyState(VK_SHIFT) < 0);
	bool bAlt = (GetKeyState(VK_MENU) < 0);

	// 1. Ctrl + Shift + Click: Select node for shortest path
	if (bCtrl && bShift)
	{
		int idx = GetClickedNodeIndex(point);
		if (idx != -1)
		{
			if (m_selectedNodePath == -1)
			{
				// First node selected
				m_selectedNodePath = idx;
				m_shortestPath.clear();
				m_selectedNodeEdge = -1;
			}
			else
			{
				// Second node selected, run Dijkstra
				RunDijkstra(m_selectedNodePath, idx);
				m_selectedNodePath = -1;
			}
			Invalidate();
		}
		return;
	}

	// 2. Ctrl + Click: Select node for edge creation
	else if (bCtrl && !bShift)
	{
		Node newNode;
		newNode.pt = ScreentoImage(point);
		newNode.id = (int)m_nodes.size();
		m_nodes.push_back(newNode);
		Invalidate();
	}

	// 3. Alt + Click: Create edge between selected node and clicked node
	else if (bAlt)
	{
		int idx = GetClickedNodeIndex(point);
		if (idx != -1)
		{
			if (m_selectedNodeEdge == -1)
			{
				// First node selected
				m_selectedNodeEdge = idx;
				m_selectedNodePath = -1;
				m_shortestPath.clear();
			}
			else
			{
				// Second node selected, create edge
				if (m_selectedNodeEdge != idx)
				{
					double dist = sqrt(pow(m_nodes[m_selectedNodeEdge].pt.x - m_nodes[idx].pt.x, 2) +
						pow(m_nodes[m_selectedNodeEdge].pt.y - m_nodes[idx].pt.y, 2));
					m_edges.push_back({ m_selectedNodeEdge, idx, dist });
				}
				m_selectedNodeEdge = -1;
			}
			Invalidate();
		}
	}

	CWnd::OnLButtonDown(nFlags, point);
}

int CChildView::GetClickedNodeIndex(CPoint pt)
{
	for (int i = 0; i < m_nodes.size(); ++i)
	{
		CPoint nodeScreenPt = ImagetoScreen(m_nodes[i].pt);

		double d = sqrt(pow(nodeScreenPt.x - pt.x, 2) + pow(nodeScreenPt.y - pt.y, 2));
		if (d <= NODE_RADIUS + 2)
		{
			return i;
		}
	}
	return -1;
}

void CChildView::RunDijkstra(int startNode, int endNode)
{
	int n = (int)m_nodes.size();
	const double INF = (numeric_limits<double>::max)();

	 vector<double> dist(n, INF);
	 vector<int> prev(n, -1);
	 priority_queue< pair<double, int>,
		 vector< pair<double, int>>,
		 greater< pair<double, int>>> pq;

	dist[startNode] = 0;
	pq.push({ 0, startNode });

	// Dijkstra's algorithm
	 vector< vector< pair<int, double>>> adj(n);
	 for (const auto& edge : m_edges)
	 {
		 adj[edge.u].push_back({ edge.v, edge.dist });
		 adj[edge.v].push_back({ edge.u, edge.dist });
	 }

	 while (!pq.empty())
	 {
		 double d = pq.top().first;
		 int u = pq.top().second;
		 pq.pop();

		 if (d > dist[u]) continue;
		 if (u == endNode) break; // Stop if we reached the end node

		 for (const auto& edge : adj[u])
		 {
			 int v = edge.first;
			 double weight = edge.second;
			 if (dist[u] + weight < dist[v])
			 {
				 dist[v] = dist[u] + weight;
				 prev[v] = u;
				 pq.push({ dist[v], v });
			 }
		 }
	 }

	 // Reconstruct shortest path
	 m_shortestPath.clear();
	 if (dist[endNode] != INF)
	 {
		 for (int v = endNode; v != -1; v = prev[v])
		 {
			 m_shortestPath.push_back(v);
		 }

		 CString msg;
		 msg.Format(_T("Shortest distance: %.2f"), dist[endNode]);
		 AfxMessageBox(msg);
	 }
	 else
	 {
		 AfxMessageBox(_T("No path found between the selected nodes."));
	 }
}
void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	// 1. Node deletion
	int nodeIdx = GetClickedNodeIndex(point);
	if (nodeIdx != -1)
	{
		if (MessageBox(_T("Delete this node and its connected edges?"), _T("Confirm Deletion"), MB_YESNO) == IDYES)

			for (int i = (int)m_edges.size() - 1; i >= 0; --i)
			{
				if (m_edges[i].u == nodeIdx || m_edges[i].v == nodeIdx)
				{
					m_edges.erase(m_edges.begin() + i);
				}
			}


		for (auto& edge : m_edges)
		{
			if (edge.u > nodeIdx) edge.u--;
			if (edge.v > nodeIdx) edge.v--;
		}

		m_nodes.erase(m_nodes.begin() + nodeIdx);

		m_selectedNodeEdge = -1;
		m_selectedNodePath = -1;
		m_shortestPath.clear();

		Invalidate();
		return;
	}
	
	// 2. Edge deletion
	int edgeIdx = GetClickedEdgeIndex(point);
	if (edgeIdx != -1)
	{
		if (MessageBox(_T("Delete this edge?"), _T("Confirm Deletion"), MB_YESNO) == IDYES)
		{
			m_edges.erase(m_edges.begin() + edgeIdx);
			m_shortestPath.clear();
			Invalidate();
		}
	}

	CWnd::OnRButtonDown(nFlags, point);
}

int CChildView::GetClickedEdgeIndex(CPoint pt)
{
	const double TOLERANCE = 5.0; // Pixel threshold for clicking near an edge

	for (int i = 0; i < m_edges.size(); ++i)
	{
		CPoint p1 = ImagetoScreen(m_nodes[m_edges[i].u].pt);
		CPoint p2 = ImagetoScreen(m_nodes[m_edges[i].v].pt);

		CRect rect(p1, p2);
		rect.NormalizeRect();
		rect.InflateRect((int)TOLERANCE, (int)TOLERANCE);

		if (!rect.PtInRect(pt)) continue;

		double cross_product = abs((double)(p2.x - p1.x) * (pt.y - p1.y) - (double)(p2.y - p1.y) * (pt.x - p1.x));
		double currentLength = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));

		if (currentLength == 0) continue;

		double dist = cross_product / currentLength;

		if (dist <= TOLERANCE)
		{
			return i;
		}
	}
	return -1;
}

CPoint CChildView::ImagetoScreen(CPoint ptImage)
{
	if (m_image.IsNull()) return ptImage;

	CRect rect;
	GetClientRect(&rect);

	double scaleX = (double)rect.Width() / m_image.GetWidth();
	double scaleY = (double)rect.Height() / m_image.GetHeight();

	return CPoint((int)(ptImage.x * scaleX), (int)(ptImage.y * scaleY));
}

CPoint CChildView::ScreentoImage(CPoint ptScreen)
{
	if (m_image.IsNull()) return ptScreen;

	CRect rect;
	GetClientRect(&rect);

	if (rect.Width() == 0 || rect.Height() == 0) return ptScreen;
	
	double scaleX = (double)m_image.GetWidth() / rect.Width();
	double scaleY = (double)m_image.GetHeight() / rect.Height();
	
	return CPoint((int)(ptScreen.x * scaleX), (int)(ptScreen.y * scaleY));
}