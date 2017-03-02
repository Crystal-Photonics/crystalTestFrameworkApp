#ifndef PLOT_H
#define PLOT_H

#include <memory>
#include <qwt_plot.h>
#include <vector>

class QAction;
class QSplitter;
class QwtPlot;
class QwtPlotCurve;
class Plot;

class Curve {
	public:
	Curve(QSplitter *, Plot *plot);
	Curve(Curve &&other) = delete;
	~Curve();
	void add(double x, double y);
	void add(const std::vector<double> &data);
	void add(const unsigned int spectrum_start_channel, const std::vector<double> &data);
	void clear();
	void set_offset(double offset);
	void set_gain(double gain);
	double integrate_ci(double integral_start_ci, double integral_end_ci);
	void set_enable_median(bool enable);
	void set_median_kernel_size(int kernel_size);
	void set_color(const QColor &color);
	void set_color_by_name(const std::string &name);
	void set_color_by_rgb(int r, int g, int b);

	private:
	void resize(std::size_t size);
	void update();
	void detach();

	Plot *plot{nullptr};
	QwtPlotCurve *curve{nullptr};
	std::vector<double> xvalues{};
	std::vector<double> yvalues_orig{};
	std::vector<double> yvalues_plot{};
	bool median_enable{false};
	unsigned int median_kernel_size{3};
	double offset{0};
	double gain{1};

	friend class Plot;
};

class Plot {
    public:
    Plot(QSplitter *parent);
	Plot(Plot &&other);
	Plot &operator=(Plot &&other);
    ~Plot();
    void clear();

	private:
	void update();
	void set_rightclick_action();

	QwtPlot *plot{nullptr};
	QAction *save_as_csv_action{nullptr};
	std::vector<Curve *> curves{};
	int curve_id_counter{0};

	friend class Curve;
};

#endif // PLOT_H
