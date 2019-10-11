R__LOAD_LIBRARY(libMathMore)

void DrawText(TString string, float y, TF1 *fit)
{
  TPaveText *pt = new TPaveText(0.2,y,0.5,y+0.05,"tbNDC");
  pt->SetBorderSize(0);
  pt->SetTextAlign(12);
  pt->AddText(string);
  pt->Draw();
  pt->SetTextColor(fit->GetLineColor());
  pt->SetTextSize(0.0375);
}

void bm_init(std::vector<std::string> paths, std::vector<int> bloatFactors)
{
   auto N = paths.size();
   auto graph = new TGraphErrors();
   float maxY = 0.0;

   for (unsigned i = 0; i < N; ++i) {
      auto f = paths[i];
      std::vector<float> realTimes;
      realTimes.resize(6);
      std::ifstream fileTiming(f.c_str());
      std::string line;
      while (std::getline(fileTiming, line))
      {
         std::istringstream iss(line);
         std::string what;
         iss >> what;
         if (what != "realtime:")
            continue;
         iss >> realTimes[0] >> realTimes[1] >> realTimes[2]
             >> realTimes[3] >> realTimes[4] >> realTimes[5];
      }

      float n = realTimes.size();
      float mean = 0.0;
      for (auto t : realTimes)
        mean += t;
      mean /= n;
      float s2 = 0.0;
      for (auto t : realTimes)
        s2 += (t - mean) * (t - mean);
      s2 /= (n - 1);
      float s = sqrt(s2);
      float t = abs(ROOT::Math::tdistribution_quantile(0.05 / 2., n - 1));
      float error = t * s / sqrt(n);
      maxY = std::max(maxY, mean + error);

      graph->SetPoint(i, bloatFactors[i], mean);
      graph->SetPointError(i, 0, error);
   }

   new TCanvas();
   graph->SetFillColor(kBlue);
   graph->SetLineColor(12);
   graph->SetLineWidth(2);
   graph->SetMarkerColor(kBlack);
   graph->GetYaxis()->SetRangeUser(0, maxY * 1.1);
   //graph->GetXaxis()->SetRangeUser(-2, bloatFactors[N-1] + 2);
   graph->Draw("ab");
   graph->Draw("p");

   auto fit = new TF1("fitThroughput", "[0] + x*[1]", bloatFactors[1], bloatFactors[N-1]);
   fit->SetLineColor(kRed);
   fit->SetLineWidth(2);
   graph->Fit(fit);

   std::cout << Form("intercept = %2.4f #pm %2.4f s", fit->GetParameter(0),
      TMath::Sqrt(fit->GetChisquare() / fit->GetNDF())) << std::endl;
   DrawText(Form("intercept = %2.4f #pm %2.4f s", fit->GetParameter(0),
      TMath::Sqrt(fit->GetChisquare() / fit->GetNDF())),
      maxY, fit);
}